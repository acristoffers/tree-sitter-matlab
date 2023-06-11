#include "tree_sitter/parser.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <wctype.h>

enum TokenType {
    COMMENT,
    COMMAND_NAME,
    COMMAND_ARGUMENT,
    STRING_OPEN,
    STRING_CLOSE,
    FORMATTING_SEQUENCE,
    ESCAPE_SEQUENCE,
    STRING_TEXT,
    MULTIVAR_OPEN,
    ERROR_SENTINEL
};

static const char* keywords[] = {
    "arguments",
    "break",
    "case",
    "catch",
    "classdef",
    "continue",
    "else",
    "elseif",
    "end",
    "enumeration",
    "events",
    "false",
    "for",
    "function",
    "global",
    "if",
    "methods",
    "otherwise",
    "parfor",
    "persistent",
    "properties",
    "return",
    "spmd",
    "switch",
    "true",
    "try",
    "while",
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
iswspace_matlab(const char c)
{
    return iswspace(c) && c != '\n' && c != '\r';
}

static inline bool
is_identifier(const char c, const bool start)
{
    const bool alpha = isalpha(c);
    const bool numeric = !start && isdigit(c);
    const bool especial = c == '_';

    return alpha || numeric || especial;
}

static inline char*
consume_identifier(TSLexer* lexer)
{
    char* identifier = calloc(256, sizeof(char));
    size_t i = 0;
    if (is_identifier(lexer->lookahead, true)) {
        identifier[i] = lexer->lookahead;
        consume(lexer);
        while (is_identifier(lexer->lookahead, false)) {
            identifier[++i] = lexer->lookahead;
            consume(lexer);
        }
        return identifier;
    }
    free(identifier);
    return NULL;
}

static inline void
skip_whitespaces(TSLexer* lexer)
{
    while (iswspace(lexer->lookahead)) {
        skip(lexer);
    }
}

static inline void
consume_whitespaces(TSLexer* lexer)
{
    while (iswspace(lexer->lookahead)) {
        consume(lexer);
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
bool is_shell_scape = false;
char string_delimiter = 0;

unsigned
tree_sitter_matlab_external_scanner_serialize(void* payload, char* buffer)
{
    buffer[0] = is_inside_command;
    buffer[1] = line_continuation;
    buffer[2] = is_shell_scape;
    buffer[3] = string_delimiter;
    return 4;
}

void tree_sitter_matlab_external_scanner_deserialize(void* payload,
    const char* buffer,
    unsigned length)
{
    if (length >= 4) {
        is_inside_command = buffer[0];
        line_continuation = buffer[1];
        is_shell_scape = buffer[2];
        string_delimiter = buffer[3];
    }
}

void consume_comment_line(TSLexer* lexer)
{
    while (lexer->lookahead != '\n' && !lexer->eof(lexer)) {
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
            consume_whitespaces(lexer);

            if (consume_char('%', lexer) && consume_char('}', lexer)) {
                lexer->result_symbol = COMMENT;
                lexer->mark_end(lexer);
                return true;
            }
        }

        return false;
    } else if (percent || line_continuation) {
        consume_comment_line(lexer);

        if (line_continuation) {
            consume(lexer);
        }

        lexer->result_symbol = COMMENT;
        lexer->mark_end(lexer);

        if (!line_continuation) {
            consume(lexer);
        }

        // Merges consecutive comments into one token, unless they are
        // separated by a newline.
        while (!lexer->eof(lexer) && (lexer->lookahead == ' ' || lexer->lookahead == '\t')) {
            consume(lexer);
        }

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
        while (iswspace_matlab(lexer->lookahead)) {
            consume(lexer);
        }
        while (lexer->lookahead != ' ' && lexer->lookahead != '\n' && !lexer->eof(lexer)) {
            consume(lexer);
        }
        lexer->result_symbol = COMMAND_NAME;
        lexer->mark_end(lexer);
        while (iswspace_matlab(lexer->lookahead)) {
            consume(lexer);
        }
        is_inside_command = lexer->lookahead != '\n';
        is_shell_scape = is_inside_command;
        return true;
    }

    if (!is_identifier(lexer->lookahead, true)) {
        return false;
    }

    char* identifier = consume_identifier(lexer);
    if (identifier != NULL) {
        for (int i = 0; i < 27; i++) {
            if (strcmp(keywords[i], identifier) == 0) {
                free(identifier);
                return false;
            }
        }
        free(identifier);
    }

    // First case: found an end-of-line already, so this is a command for sure.
    // example:
    // pwd
    // pwd;
    // pwd,
    if (is_eol(lexer->lookahead)) {
        lexer->result_symbol = COMMAND_NAME;
        lexer->mark_end(lexer);
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
    lexer->result_symbol = COMMAND_NAME;
    lexer->mark_end(lexer);
    consume_whitespaces(lexer);

    // Check for end-of-line again, since it may be that the user just put a
    // space at the end, like `pwd ;`
    if (is_eol(lexer->lookahead)) {
        return true;
    }

    // The first char of the first argument cannot be /=()/
    if (lexer->lookahead == '=' || lexer->lookahead == '(' || lexer->lookahead == ')') {
        return false;
    }

    // If it is a single quote, it is a command.
    if (lexer->lookahead == '\'') {
        is_inside_command = true;
        return true;
    }

    // If it is an identifier char, then it's a command
    if (is_identifier(lexer->lookahead, false)) {
        is_inside_command = true;
        return true;
    }

    // If it is a char greater than 0xC0, then I assume it's a valide UTF-8
    // char, and this is a command.
    if (lexer->lookahead >= 0xC0) {
        is_inside_command = true;
        return true;
    }

    // Let's now consider punctuation marks.
    if (ispunct(lexer->lookahead)) {
        // In this case, we advance and look at what comes next too.
        const char first = lexer->lookahead;
        consume(lexer);
        const char second = lexer->lookahead;

        // If it's the end-of-line, then it's a command.
        if (is_eol(second)) {
            is_inside_command = true;
            return true;
        }

        if (iswspace_matlab(second)) {
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
                consume(lexer);
                while (iswspace_matlab(lexer->lookahead)) {
                    consume(lexer);
                }
                is_inside_command = is_eol(lexer->lookahead);
                return is_inside_command;
            }

            // If it's not an operator, then this is a command.
            is_inside_command = true;
            return true;
        }

        // Now we check for the rest of the operators.
        // Since they have 2 digits, it matters if the next is a space.
        consume(lexer);

        if (lexer->lookahead != ' ') {
            is_inside_command = true;
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

        is_inside_command = true;
        return true;
    }

    return false;
}

bool scan_command_argument(TSLexer* lexer)
{
    if (is_shell_scape) {
        while (lexer->lookahead != ' ' && lexer->lookahead != '\n' && !lexer->eof(lexer)) {
            consume(lexer);
        }
        lexer->result_symbol = COMMAND_ARGUMENT;
        lexer->mark_end(lexer);
        while (iswspace_matlab(lexer->lookahead)) {
            consume(lexer);
        }
        if (lexer->lookahead == '\n') {
            is_inside_command = false;
            is_shell_scape = false;
        }
        return true;
    }

    const char nesting_open[] = { '\'', '(', '{', '[' };
    const char nesting_close[] = { '\'', ')', '}', ']' };

    bool nesting = false;
    char nesting_char = 0;
    bool consumed = false;

    while (!lexer->eof(lexer)) {
        if ((is_eol(lexer->lookahead) || iswspace_matlab(lexer->lookahead)) && !nesting || nesting && nesting_char != '\'' && lexer->lookahead == ';') {
            lexer->result_symbol = COMMAND_ARGUMENT;
            lexer->mark_end(lexer);

            while (iswspace_matlab(lexer->lookahead)) {
                consume(lexer);
            }

            if (is_eol(lexer->lookahead)) {
                is_inside_command = false;
            }

            return true;
        }

        // Line comment, finish.
        if (lexer->lookahead == '%' && nesting_char != '\'') {
            is_inside_command = false;
            if (consumed) {
                lexer->result_symbol = COMMAND_ARGUMENT;
                lexer->mark_end(lexer);
                return true;
            } else {
                // At this point we actually know it will return true,
                // so is_inside_command will be saved.
                return scan_comment(lexer);
            }
        }

        // Line continuation
        if (lexer->lookahead == '.') {
            lexer->result_symbol = COMMAND_ARGUMENT;
            lexer->mark_end(lexer);
            consume(lexer);
            if (lexer->lookahead == '.') {
                consume(lexer);
                if (lexer->lookahead == '.') {
                    if (consumed) {
                        line_continuation = true;
                    } else {
                        consume_comment_line(lexer);
                        lexer->result_symbol = COMMENT;
                        lexer->mark_end(lexer);
                    }
                    return true;
                }
                consumed = true;
                continue;
            }
            consumed = true;
            continue;
        }

        for (int i = 0; i < sizeof(nesting_open); i++) {
            if (lexer->lookahead == nesting_open[i] && !nesting) {
                nesting = true;
                nesting_char = lexer->lookahead;
                goto CONTINUE_FOR;
            }
        }

        for (int i = 0; i < sizeof(nesting_close); i++) {
            if (lexer->lookahead == nesting_close[i] && nesting && nesting_char == nesting_open[i]) {
                nesting = false;
                nesting_char = 0;
                break;
            }
        }

    CONTINUE_FOR:
        consume(lexer);
        consumed = true;
    }

    return false;
}

bool scan_string_open(TSLexer* lexer)
{
    if (lexer->lookahead == '"') {
        string_delimiter = lexer->lookahead;
        consume(lexer);
        lexer->result_symbol = STRING_OPEN;
        lexer->mark_end(lexer);
        return true;
    }

    return false;
}

bool scan_string_close(TSLexer* lexer)
{
    if (lexer->lookahead == string_delimiter) {
        consume(lexer);
        string_delimiter = 0;
        lexer->result_symbol = STRING_CLOSE;
        lexer->mark_end(lexer);
        return true;
    }

    if (lexer->lookahead == '%') {
        consume(lexer);

        if (lexer->lookahead == '%') {
            consume(lexer);
            lexer->result_symbol = FORMATTING_SEQUENCE;
            lexer->mark_end(lexer);
            return true;
        }

        const char* valid_tokens = "1234567890.-+ #btcdeEfgGosuxX";
        const char* end_tokens = "cdeEfgGosuxX";
        while (!lexer->eof(lexer) && lexer->lookahead != '\n' && lexer->lookahead != '\r') {
            bool is_valid = false;
            for (int i = 0; i < 29; i++) {
                if (valid_tokens[i] == lexer->lookahead) {
                    is_valid = true;
                    break;
                }
            }

            if (!is_valid) {
                lexer->result_symbol = FORMATTING_SEQUENCE;
                lexer->mark_end(lexer);
                return true;
            }

            for (int i = 0; i < 12; i++) {
                if (end_tokens[i] == lexer->lookahead) {
                    consume(lexer);
                    lexer->result_symbol = FORMATTING_SEQUENCE;
                    lexer->mark_end(lexer);
                    return true;
                }
            }

            consume(lexer);
        }

        string_delimiter = 0;
        return false;
    }

    if (lexer->lookahead == '\\') {
        consume(lexer);

        if (lexer->lookahead == 'x') {
            consume(lexer);
            while (!lexer->eof(lexer)) {
                const char* hexa_chars = "1234567890abcdefABCDEF";
                bool is_valid = false;
                for (int i = 0; i < 22; i++) {
                    if (hexa_chars[i] == lexer->lookahead) {
                        is_valid = true;
                        break;
                    }
                }

                if (!is_valid) {
                    lexer->result_symbol = ESCAPE_SEQUENCE;
                    lexer->mark_end(lexer);
                    return true;
                }

                consume(lexer);
            }
        }

        if (lexer->lookahead >= '0' && lexer->lookahead <= '7') {
            while (lexer->lookahead >= '0' && lexer->lookahead <= '7' && !lexer->eof(lexer)) {
                consume(lexer);
            }

            lexer->result_symbol = ESCAPE_SEQUENCE;
            lexer->mark_end(lexer);
            return true;
        }

        const char* escapes = "abfnrtv\\";
        bool is_valid = false;
        for (int i = 0; i < 8; i++) {
            if (escapes[i] == lexer->lookahead) {
                is_valid = true;
                break;
            }
        }

        if (is_valid) {
            consume(lexer);
            lexer->result_symbol = ESCAPE_SEQUENCE;
            lexer->mark_end(lexer);
            return true;
        }

        string_delimiter = 0;
        return false;
    }

    while (lexer->lookahead != '\n' && lexer->lookahead != '\r' && !lexer->eof(lexer)) {
        // In MATLAB '' and "" are valid inside their own kind: 'It''s ok' "He said ""it's ok"""
        if (lexer->lookahead == string_delimiter) {
            lexer->result_symbol = STRING_TEXT;
            lexer->mark_end(lexer);
            consume(lexer);
            if (lexer->lookahead != string_delimiter) {
                return true;
            }
            consume(lexer);
            continue;
        }

        // The scanner will be called again, and this time we will match in the if
        // before this while.
        if (lexer->lookahead == '%' || lexer->lookahead == '\\') {
            lexer->result_symbol = STRING_TEXT;
            lexer->mark_end(lexer);
            return true;
        }

        consume(lexer);
    }

    string_delimiter = 0;
    return false;
}

bool scan_multivar_open(TSLexer* lexer)
{
    consume(lexer);
    lexer->result_symbol = MULTIVAR_OPEN;
    lexer->mark_end(lexer);

    while (!lexer->eof(lexer) && lexer->lookahead != ']' && lexer->lookahead != '\n' && lexer->lookahead != '\r') {
        consume(lexer);
    }

    if (lexer->lookahead != ']') {
        return false;
    }

    consume(lexer);

    while (!lexer->eof(lexer) && iswspace_matlab(lexer->lookahead)) {
        consume(lexer);
    }

    if (lexer->lookahead == '=') {
        return true;
    }

    return false;
}

bool tree_sitter_matlab_external_scanner_scan(void* payload,
    TSLexer* lexer,
    const bool* valid_symbols)
{
    skip_whitespaces(lexer);

    if (string_delimiter == 0) {
        if ((line_continuation || !is_inside_command) && valid_symbols[COMMENT] && (lexer->lookahead == '%' || lexer->lookahead == '.')) {
            return scan_comment(lexer);
        }

        if (valid_symbols[STRING_OPEN] && (lexer->lookahead == '\'' || lexer->lookahead == '"')) {
            return scan_string_open(lexer);
        }

        if (valid_symbols[MULTIVAR_OPEN] && !is_inside_command && !line_continuation && lexer->lookahead == '[') {
            return scan_multivar_open(lexer);
        }

        if (valid_symbols[COMMAND_NAME] && !is_inside_command) {
            is_inside_command = false;
            is_shell_scape = false;
            return scan_command(lexer);
        }

        if (valid_symbols[COMMAND_ARGUMENT] && is_inside_command) {
            return scan_command_argument(lexer);
        }
    } else {
        if (valid_symbols[STRING_CLOSE] || valid_symbols[FORMATTING_SEQUENCE] || valid_symbols[ESCAPE_SEQUENCE]) {
            return scan_string_close(lexer);
        }
    }

    return false;
}
