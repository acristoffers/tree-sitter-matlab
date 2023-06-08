#include <ctype.h>
#include <stdio.h>
#include <tree_sitter/parser.h>
#include <wctype.h>

enum TokenType {
    COMMENT,
    COMMAND_NAME,
    COMMAND_ARGUMENT,
    ERROR_SENTINEL
};

static inline void
consume(TSLexer* lexer)
{
    lexer->advance(lexer, false);
}

static inline void
skip(TSLexer* lexer)
{
    lexer->advance(lexer, true);
}

static inline bool
consume_char(char c, TSLexer* lexer)
{
    if (lexer->lookahead != c) {
        return false;
    }
    consume(lexer);
    return true;
}

static inline bool
is_eol(const char c)
{
    return c == '\n' || c == '\r' || c == ',' || c == ';';
}

static inline bool
is_identifier(const char c, const bool start)
{
    const bool alpha = isalpha(c);
    const bool numeric = !start && isdigit(c);
    const bool especial = c == '_';

    return alpha || numeric || especial;
}

static inline void
consume_identifier(TSLexer* lexer)
{
    is_identifier(lexer->lookahead, true);
    while (is_identifier(lexer->lookahead, false)) {
        consume(lexer);
    }
}

static inline void
skip_whitespaces(TSLexer* lexer)
{
    while (iswspace(lexer->lookahead)) {
        skip(lexer);
    }
}

void* tree_sitter_matlab_external_scanner_create()
{
    return NULL;
}

void tree_sitter_matlab_external_scanner_destroy(void* payload)
{
}

bool is_inside_command = false;
bool line_continuation = false;

unsigned
tree_sitter_matlab_external_scanner_serialize(void* payload, char* buffer)
{
    buffer[0] = is_inside_command;
    buffer[1] = line_continuation;
    return 2;
}

void tree_sitter_matlab_external_scanner_deserialize(void* payload,
    const char* buffer,
    unsigned length)
{
    if (length >= 2) {
        is_inside_command = buffer[0];
        line_continuation = buffer[1];
    }
}

bool consume_comment_line(TSLexer* lexer)
{
    while (lexer->lookahead != '\n') {
        consume(lexer);
    }
}

bool scan_comment(TSLexer* lexer)
{
    const bool percent = lexer->lookahead == '%';
    const bool block = percent && consume_char('%', lexer) && consume_char('{', lexer);
    const bool line_continuation = lexer->lookahead == '.' && consume_char('.', lexer) && consume_char('.', lexer) && consume_char('.', lexer);

    if (block) {
        while (!lexer->eof(lexer)) {
            consume_comment_line(lexer);
            consume(lexer);
            skip_whitespaces(lexer);

            if (consume_char('%', lexer) && consume_char('}', lexer)) {
                lexer->mark_end(lexer);
                lexer->result_symbol = COMMENT;
                return true;
            }
        }

        return false;
    } else if (percent || line_continuation) {
        consume_comment_line(lexer);

        lexer->mark_end(lexer);
        lexer->result_symbol = COMMENT;

        // Merges consecutive comments into one token, unless they are
        // separated by a newline.
        consume(lexer);
        if (lexer->lookahead == '%') {
            return scan_comment(lexer);
        }

        return true;
    }

    return false;
}

bool scan_command(TSLexer* lexer)
{
    // Special case: shell escape
    if (lexer->lookahead == '!') {
        consume(lexer);
        skip_whitespaces(lexer);
        consume_identifier(lexer);
        lexer->mark_end(lexer);
        lexer->result_symbol = COMMAND_NAME;
        return true;
    }

    if (!is_identifier(lexer->lookahead, true)) {
        return false;
    }

    consume_identifier(lexer);

    // First case: found an end-of-line already, so this is a command for sure.
    // example:
    // pwd
    // pwd;
    // pwd,
    if (is_eol(lexer->lookahead)) {
        lexer->mark_end(lexer);
        lexer->result_symbol = COMMAND_NAME;
        return true;
    }

    // If it's not followed by a space, it may be something else, like A' for
    // example. Or A+2.
    if (lexer->lookahead != ' ') {
        return false;
    }

    // If it is followed by a space, it doesn't mean it's a command yet.
    // It could be A + 2 or A = 2. Let's check what is the first char after
    // all whitespaces. We mark it already as this is the right place, and we
    // only need to make sure this is a command and not something else from
    // this point on.
    lexer->mark_end(lexer);
    lexer->result_symbol = COMMAND_NAME;
    skip_whitespaces(lexer);

    // Check for end-of-line again, since it may be that the user just put a
    // space at the end, like `pwd ;`
    if (is_eol(lexer->lookahead)) {
        lexer->mark_end(lexer);
        lexer->result_symbol = COMMAND_NAME;
        return true;
    }

    // From now on, if it is a command, it has arguments.
    is_inside_command = true;

    // The first char of the first argument cannot be /=()/
    if (lexer->lookahead == '=' || lexer->lookahead == '(' || lexer->lookahead == ')') {
        return false;
    }

    // If it is a single quote, it is a command.
    if (lexer->lookahead == '\'') {
        return true;
    }

    // If it is an identifier char, then it's a command
    if (is_identifier(lexer->lookahead, true)) {
        return true;
    }

    // If it is a char greater than 0xC0, then I assume it's a valide UTF-8
    // char, and this is a command.
    if (lexer->lookahead >= 0xC0) {
        return true;
    }

    // Let's now consider punctuation marks.
    if (ispunct(lexer->lookahead)) {
        // In this case, we advance and look at what comes next too.
        const char first = lexer->lookahead;
        skip(lexer);
        const char second = lexer->lookahead;

        // If it's the end-of-line, then it's a command.
        if (is_eol(second)) {
            return true;
        }

        if (second == ' ') {
            // If it is a space, then it depends on what we have, since
            // `disp + ;` is a valid command but `disp + 2;` isn't.
            const char operators[] = {
                '!',
                '&',
                '*',
                '+',
                '-',
                '/',
                '<',
                '>',
                '@',
                '\\',
                '^',
                '|',
            };
            bool is_invalid = false;
            for (int i = 0; i < sizeof(operators); i++) {
                if (first == operators[i]) {
                    is_invalid = true;
                    break;
                }
            }
            // If it is an operator, this can only be a command if there
            // are no further arguments.
            if (is_invalid) {
                skip(lexer);
                while (lexer->lookahead == ' ') {
                    skip(lexer);
                }
                return is_eol(lexer->lookahead);
            }

            // If it's not an operator, then this is a command.
            return true;
        }

        // Now we check for the rest of the operators.
        // Since they have 2 digits, it matters if the next is a space.
        skip(lexer);

        if (lexer->lookahead != ' ') {
            return true;
        }

        const char operators[][2] = {
            { '&', '&' },
            { '|', '|' },
            { '=', '=' },
            { '~', '=' },
            { '<', '=' },
            { '>', '=' },
            { '.', '+' },
            { '.', '-' },
            { '.', '*' },
            { '.', '/' },
            { '.', '\\' },
            { '.', '^' },
        };

        for (int i = 0; i < sizeof(operators); i++) {
            if (operators[i][0] == first && operators[i][1] == second) {
                return false;
            }
        }

        return true;
    }

    return false;
}

bool scan_command_argument(TSLexer* lexer)
{
    const char nesting_open[] = { '\'', '(', '{', '[' };
    const char nesting_close[] = { '\'', ')', '}', ']' };

    bool nesting = false;
    char nesting_char = 0;

    while (!lexer->eof(lexer)) {
        if ((is_eol(lexer->lookahead) || lexer->lookahead == ' ') && !nesting || nesting && nesting_char != '\'' && lexer->lookahead == ';') {
            lexer->mark_end(lexer);
            lexer->result_symbol = COMMAND_ARGUMENT;

            while (lexer->lookahead == ' ') {
                skip(lexer);
            }

            if (is_eol(lexer->lookahead)) {
                is_inside_command = false;
            }

            return true;
        }

        // Line comment, finish.
        if (lexer->lookahead == '%' && nesting_char != '\'') {
            lexer->mark_end(lexer);
            lexer->result_symbol = COMMAND_ARGUMENT;
            is_inside_command = false;
            return true;
        }

        // Line continuation
        if (lexer->lookahead == '.') {
            lexer->mark_end(lexer);
            lexer->result_symbol = COMMAND_ARGUMENT;
            consume(lexer);
            if (lexer->lookahead == '.') {
                consume(lexer);
                if (lexer->lookahead == '.') {
                    line_continuation = true;
                    return true;
                }
                continue;
            }
        }

        for (int i = 0; i < sizeof(nesting_open); i++) {
            if (lexer->lookahead == nesting_open[i] && !nesting) {
                nesting = true;
                nesting_char = lexer->lookahead;
                break;
            }
        }

        for (int i = 0; i < sizeof(nesting_close); i++) {
            if (lexer->lookahead == nesting_close[i] && nesting && nesting_char == nesting_open[i]) {
                nesting = false;
                nesting_char = 0;
                break;
            }
        }

        consume(lexer);
    }

    return false;
}

bool tree_sitter_matlab_external_scanner_scan(void* payload,
    TSLexer* lexer,
    const bool* valid_symbols)
{
    skip_whitespaces(lexer);

    if ((line_continuation || !is_inside_command) && valid_symbols[COMMENT] && (lexer->lookahead == '%' || lexer->lookahead == '.')) {
        return scan_comment(lexer);
    }

    if (valid_symbols[COMMAND_NAME] && !is_inside_command) {
        is_inside_command = false;
        return scan_command(lexer);
    }

    if (valid_symbols[COMMAND_ARGUMENT] && is_inside_command) {
        return scan_command_argument(lexer);
    }

    return false;
}
