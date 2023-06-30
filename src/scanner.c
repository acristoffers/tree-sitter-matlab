#include "tree_sitter/parser.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <wctype.h>

enum TokenType {
    COMMENT,
    LINE_CONTINUATION,
    COMMAND_NAME,
    COMMAND_ARGUMENT,
    SINGLE_QUOTE_STRING_START,
    SINGLE_QUOTE_STRING_END,
    DOUBLE_QUOTE_STRING_START,
    DOUBLE_QUOTE_STRING_END,
    FORMATTING_SEQUENCE,
    ESCAPE_SEQUENCE,
    STRING_CONTENT,
    ENTRY_DELIMITER,
    MULTIOUTPUT_VARIABLE_START,
    ERROR_SENTINEL,
    EOF_,
};

typedef struct {
    bool is_inside_command;
    bool line_continuation;
    bool is_shell_scape;
    char string_delimiter;
} Scanner;

static const char* const keywords[] = {
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
advance(TSLexer* lexer)
{
    lexer->advance(lexer, false);
}

static inline void
skip(TSLexer* lexer)
{
    lexer->advance(lexer, true);
}

static inline bool
consume_char(char chr, TSLexer* lexer)
{
    if (lexer->lookahead != chr) {
        return false;
    }
    advance(lexer);
    return true;
}

static inline bool
is_eol(const char chr)
{
    return chr == '\n' || chr == '\r' || chr == ',' || chr == ';';
}

static inline bool
iswspace_matlab(const char chr)
{
    return iswspace(chr) && chr != '\n' && chr != '\r';
}

static inline bool
is_identifier(const char chr, const bool start)
{
    const bool alpha = isalpha(chr);
    const bool numeric = !start && isdigit(chr);
    const bool special = chr == '_';

    return alpha || numeric || special;
}

static inline void
consume_identifier(TSLexer* lexer, char* buffer)
{
    size_t size = 0;
    if (is_identifier(lexer->lookahead, true)) {
        buffer[size] = (char)lexer->lookahead;
        advance(lexer);
        while (is_identifier(lexer->lookahead, false)) {
            if (size == 255) {
                buffer[0] = 0;
                return;
            }
            buffer[++size] = (char)lexer->lookahead;
            advance(lexer);
        }
        return;
    }
    buffer[0] = 0;
}

static inline int
skip_whitespaces(TSLexer* lexer)
{
    int skipped = 0;
    while (iswspace(lexer->lookahead)) {
        if (lexer->lookahead == '\n' || lexer->lookahead == '\r') {
            skipped = skipped | 2;
        }
        skip(lexer);
        skipped = skipped | 1;
    }
    return skipped;
}

static inline void
consume_whitespaces(TSLexer* lexer)
{
    while (iswspace(lexer->lookahead)) {
        advance(lexer);
    }
}

/* bool is_inside_command = false; */
/* bool line_continuation = false; */
/* bool is_shell_scape = false; */
/* char string_delimiter = 0; */

void* tree_sitter_matlab_external_scanner_create()
{
    Scanner* scanner = calloc(1, sizeof(Scanner));
    return scanner;
}

void tree_sitter_matlab_external_scanner_destroy(void* payload)
{
}

unsigned
tree_sitter_matlab_external_scanner_serialize(void* payload, char* buffer)
{
    Scanner* scanner = (Scanner*)payload;
    buffer[0] = scanner->is_inside_command;
    buffer[1] = scanner->line_continuation;
    buffer[2] = scanner->is_shell_scape;
    buffer[3] = scanner->string_delimiter;
    return 4;
}

void tree_sitter_matlab_external_scanner_deserialize(void* payload,
    const char* buffer,
    unsigned length)
{
    Scanner* scanner = (Scanner*)payload;
    if (length == 4) {
        scanner->is_inside_command = buffer[0];
        scanner->line_continuation = buffer[1];
        scanner->is_shell_scape = buffer[2];
        scanner->string_delimiter = buffer[3];
    }
}

static inline void consume_comment_line(TSLexer* lexer)
{
    while (lexer->lookahead != '\n' && lexer->lookahead != '\r' && !lexer->eof(lexer)) {
        advance(lexer);
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
            advance(lexer);
            consume_whitespaces(lexer);

            if (consume_char('%', lexer) && consume_char('}', lexer)) {
                lexer->result_symbol = COMMENT;
                lexer->mark_end(lexer);
                return true;
            }
        }

        return false;
    }
    if (percent || line_continuation) {
        consume_comment_line(lexer);

        if (line_continuation) {
            advance(lexer);
        }

        lexer->mark_end(lexer);

        if (!line_continuation) {
            lexer->result_symbol = COMMENT;
            advance(lexer);
        } else {
            while (lexer->lookahead == '\r' || lexer->lookahead == '\n') {
                advance(lexer);
            }
            lexer->mark_end(lexer);
            lexer->result_symbol = LINE_CONTINUATION;
        }

        // Merges consecutive comments into one token, unless they are
        // separated by a newline.
        while (!lexer->eof(lexer) && (lexer->lookahead == ' ' || lexer->lookahead == '\t')) {
            advance(lexer);
        }

        if (lexer->lookahead == '%') {
            return scan_comment(lexer);
        }

        return true;
    }

    return false;
}

bool scan_command(Scanner* scanner, TSLexer* lexer)
{
    // Special case: shell escape
    if (lexer->lookahead == '!') {
        advance(lexer);
        while (iswspace_matlab(lexer->lookahead)) {
            advance(lexer);
        }
        while (lexer->lookahead != ' ' && lexer->lookahead != '\n' && !lexer->eof(lexer)) {
            advance(lexer);
        }
        lexer->result_symbol = COMMAND_NAME;
        lexer->mark_end(lexer);
        while (iswspace_matlab(lexer->lookahead)) {
            advance(lexer);
        }
        scanner->is_inside_command = lexer->lookahead != '\n';
        scanner->is_shell_scape = scanner->is_inside_command;
        return true;
    }

    if (!is_identifier(lexer->lookahead, true)) {
        return false;
    }

    char buffer[256] = { 0 };
    consume_identifier(lexer, buffer);
    if (buffer[0] != 0) {
        for (int i = 0; i < 27; i++) {
            if (strcmp(keywords[i], buffer) == 0) {
                return false;
            }
        }
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
        scanner->is_inside_command = true;
        return true;
    }

    // If it is an identifier char, then it's a command
    if (is_identifier(lexer->lookahead, false)) {
        scanner->is_inside_command = true;
        return true;
    }

    // If it is a char greater than 0xC0, then assume it's a valid UTF-8
    // char, and that this is a command.
    if (lexer->lookahead >= 0xC0) {
        scanner->is_inside_command = true;
        return true;
    }

    // Let's now consider punctuation marks.
    if (ispunct(lexer->lookahead)) {
        // In this case, we advance and look at what comes next too.
        const char first = lexer->lookahead;
        advance(lexer);
        const char second = lexer->lookahead;

        // If it's the end-of-line, then it's a command.
        if (is_eol(second)) {
            scanner->is_inside_command = true;
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
                advance(lexer);
                while (iswspace_matlab(lexer->lookahead)) {
                    advance(lexer);
                }
                scanner->is_inside_command = is_eol(lexer->lookahead);
                return scanner->is_inside_command;
            }

            // If it's not an operator, then this is a command.
            scanner->is_inside_command = true;
            return true;
        }

        // Now we check for the rest of the operators.
        // Since they have 2 digits, it matters if the next is a space.
        advance(lexer);

        if (lexer->lookahead != ' ') {
            scanner->is_inside_command = true;
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

        for (int i = 0; i < 12; i++) {
            if (operators[i][0] == first && operators[i][1] == second) {
                return false;
            }
        }

        scanner->is_inside_command = true;
        return true;
    }

    return false;
}

bool scan_command_argument(Scanner* scanner, TSLexer* lexer)
{
    if (scanner->is_shell_scape) {
        while (lexer->lookahead != ' ' && lexer->lookahead != '\n' && !lexer->eof(lexer)) {
            advance(lexer);
        }
        lexer->result_symbol = COMMAND_ARGUMENT;
        lexer->mark_end(lexer);
        while (iswspace_matlab(lexer->lookahead)) {
            advance(lexer);
        }
        if (lexer->lookahead == '\n') {
            scanner->is_inside_command = false;
            scanner->is_shell_scape = false;
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
                advance(lexer);
            }

            if (is_eol(lexer->lookahead)) {
                scanner->is_inside_command = false;
            }

            return true;
        }

        // Line comment, finish.
        if (lexer->lookahead == '%' && nesting_char != '\'') {
            scanner->is_inside_command = false;
            if (consumed) {
                lexer->result_symbol = COMMAND_ARGUMENT;
                lexer->mark_end(lexer);
                return true;
            }
            return scan_comment(lexer);
        }

        // Line continuation
        if (lexer->lookahead == '.') {
            lexer->result_symbol = COMMAND_ARGUMENT;
            lexer->mark_end(lexer);
            advance(lexer);
            if (lexer->lookahead == '.') {
                advance(lexer);
                if (lexer->lookahead == '.') {
                    if (consumed) {
                        scanner->line_continuation = true;
                    } else {
                        consume_comment_line(lexer);
                        lexer->result_symbol = LINE_CONTINUATION;
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
        advance(lexer);
        consumed = true;
    }

    return false;
}

bool scan_string_open(Scanner* scanner, TSLexer* lexer)
{
    switch (lexer->lookahead) {
    case '"':
        scanner->string_delimiter = lexer->lookahead;
        advance(lexer);
        lexer->result_symbol = DOUBLE_QUOTE_STRING_START;
        lexer->mark_end(lexer);
        return true;
    case '\'':
        scanner->string_delimiter = lexer->lookahead;
        advance(lexer);
        lexer->result_symbol = SINGLE_QUOTE_STRING_START;
        lexer->mark_end(lexer);
        return true;
    default:
        return false;
    }
}

bool scan_string_close(Scanner* scanner, TSLexer* lexer)
{
    if (lexer->lookahead == scanner->string_delimiter) {
        advance(lexer);
        if (lexer->lookahead == scanner->string_delimiter) {
            advance(lexer);
            lexer->result_symbol = STRING_CONTENT;
            goto content;
        }
        lexer->result_symbol = scanner->string_delimiter == '"' ? DOUBLE_QUOTE_STRING_END : SINGLE_QUOTE_STRING_END;
        lexer->mark_end(lexer);
        scanner->string_delimiter = 0;
        return true;
    }

    if (lexer->lookahead == '%') {
        advance(lexer);

        if (lexer->lookahead == '%') {
            advance(lexer);
            lexer->result_symbol = FORMATTING_SEQUENCE;
            lexer->mark_end(lexer);
            return true;
        }

        const char* valid_tokens = "1234567890.-+ #btcdeEfgGosuxX";
        const char* end_tokens = "cdeEfgGosuxX";
        while (!lexer->eof(lexer) && lexer->lookahead != '\n' && lexer->lookahead != '\r') {
            bool is_valid = false;
            for (int i = 0; i < strlen(valid_tokens); i++) {
                if (valid_tokens[i] == lexer->lookahead) {
                    is_valid = true;
                    break;
                }
            }

            if (!is_valid) {
                lexer->result_symbol = STRING_CONTENT;
                goto content;
            }

            for (int i = 0; i < 12; i++) {
                if (end_tokens[i] == lexer->lookahead) {
                    advance(lexer);
                    lexer->result_symbol = FORMATTING_SEQUENCE;
                    lexer->mark_end(lexer);
                    return true;
                }
            }

            advance(lexer);
        }

        scanner->string_delimiter = 0;
        return false;
    }

    if (lexer->lookahead == '\\') {
        advance(lexer);

        if (lexer->lookahead == 'x') {
            advance(lexer);
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

                advance(lexer);
            }
        }

        if (lexer->lookahead >= '0' && lexer->lookahead <= '7') {
            while (lexer->lookahead >= '0' && lexer->lookahead <= '7' && !lexer->eof(lexer)) {
                advance(lexer);
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
            advance(lexer);
            lexer->result_symbol = ESCAPE_SEQUENCE;
            lexer->mark_end(lexer);
            return true;
        }
    }

content:
    while (lexer->lookahead != '\n' && lexer->lookahead != '\r' && !lexer->eof(lexer)) {
        // In MATLAB '' and "" are valid inside their own kind: 'It''s ok' "He said ""it's ok"""
        if (lexer->lookahead == scanner->string_delimiter) {
            lexer->result_symbol = STRING_CONTENT;
            lexer->mark_end(lexer);
            advance(lexer);
            if (lexer->lookahead != scanner->string_delimiter) {
                return true;
            }
            advance(lexer);
            continue;
        }

        // The scanner will be called again, and this time we will match in the if
        // before this while.
        if (lexer->lookahead == '%' || lexer->lookahead == '\\') {
            lexer->result_symbol = STRING_CONTENT;
            lexer->mark_end(lexer);
            advance(lexer);
            if (lexer->lookahead == scanner->string_delimiter || iswspace_matlab(lexer->lookahead)) {
                goto content;
            }
            return true;
        }

        advance(lexer);
    }

    scanner->string_delimiter = 0;
    return false;
}

static inline bool scan_multioutput_var_start(TSLexer* lexer)
{
    advance(lexer);
    lexer->result_symbol = MULTIOUTPUT_VARIABLE_START;
    lexer->mark_end(lexer);

    while (!lexer->eof(lexer) && lexer->lookahead != ']' && lexer->lookahead != '\n' && lexer->lookahead != '\r') {
        advance(lexer);
    }

    if (lexer->lookahead != ']') {
        return false;
    }

    advance(lexer);

    while (!lexer->eof(lexer) && iswspace_matlab(lexer->lookahead)) {
        advance(lexer);
    }

    if (lexer->lookahead == '=') {
        advance(lexer);
        if (lexer->lookahead != '=') {
            return true;
        }
    }

    return false;
}

bool scan_entry_delimiter(TSLexer* lexer, int skipped)
{
    lexer->mark_end(lexer);
    lexer->result_symbol = ENTRY_DELIMITER;

    if (skipped & 2) {
        return false;
    }

    if (lexer->lookahead == ',') {
        advance(lexer);
        lexer->mark_end(lexer);
        lexer->result_symbol = ENTRY_DELIMITER;
        return true;
    }

    if (lexer->lookahead == '.') {
        return isdigit(lexer->lookahead);
    }

    if (lexer->lookahead == '{' || lexer->lookahead == '(' || lexer->lookahead == '\'' ) {
        return skipped != 0;
    }

    if (lexer->lookahead == '[') {
        return true;
    }

    // These chars mean we cannot end the cell here, as the expression will
    // surely continue OR we need to just leave the char there and the internal
    // parser will do the rest.
    const char no_end[] = { ']', '}', '&', '|', '=', '<', '>', '*', '/', '\\', '^', ';', ':'};
    for (int i = 0; i < sizeof(no_end); i++) {
        if (no_end[i] == lexer->lookahead) {
            return false;
        }
    }

    if (lexer->lookahead == '~') {
        advance(lexer);
        return lexer->lookahead != '=';
    }

    const char maybe_end[] = { '+', '-' };
    for (int i = 0; i < sizeof(maybe_end); i++) {
        if (maybe_end[i] == lexer->lookahead) {
            advance(lexer);
            if (lexer->lookahead == ' ') {
                return false;
            }
            return skipped != 0;
        }
    }

    return skipped != 0;
}

bool tree_sitter_matlab_external_scanner_scan(void* payload,
    TSLexer* lexer,
    const bool* valid_symbols)
{
    Scanner* scanner = (Scanner*)payload;
    if (scanner->string_delimiter == 0) {
        int skipped = skip_whitespaces(lexer);

        if (
            (scanner->line_continuation || !scanner->is_inside_command)
            && valid_symbols[COMMENT]
            && (lexer->lookahead == '%' || lexer->lookahead == '.')) {
            return scan_comment(lexer);
        }

        if (
            (valid_symbols[SINGLE_QUOTE_STRING_START] && lexer->lookahead == '\'')
            || (valid_symbols[DOUBLE_QUOTE_STRING_START] && lexer->lookahead == '"')) {
            return scan_string_open(scanner, lexer);
        }

        if (!scanner->is_inside_command) {
            if (!scanner->line_continuation) {
                if (valid_symbols[MULTIOUTPUT_VARIABLE_START] && lexer->lookahead == '[') {
                    return scan_multioutput_var_start(lexer);
                }

                if (valid_symbols[ENTRY_DELIMITER]) {
                    return scan_entry_delimiter(lexer, skipped);
                }
            }

            if (valid_symbols[COMMAND_NAME]) {
                scanner->is_inside_command = false;
                scanner->is_shell_scape = false;
                return scan_command(scanner, lexer);
            }

            if (valid_symbols[ENTRY_DELIMITER] && !scanner->is_inside_command && !scanner->line_continuation) {
                return scan_entry_delimiter(lexer, skipped);
            }
        } else {
            if (valid_symbols[COMMAND_ARGUMENT]) {
                return scan_command_argument(scanner, lexer);
            }
        }
    } else {
        if (valid_symbols[DOUBLE_QUOTE_STRING_END] || valid_symbols[SINGLE_QUOTE_STRING_END] || valid_symbols[FORMATTING_SEQUENCE]) {
            return scan_string_close(scanner, lexer);
        }
    }

    if (valid_symbols[EOF_]) {
        lexer->result_symbol = EOF_;
        return lexer->eof(lexer);
    }

    return false;
}
