#include <stdio.h>
#include <tree_sitter/parser.h>
#include <wctype.h>

enum TokenType {
    COMMENT,
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

char ending_char = 0;
uint8_t level_count = 0;

static inline void
reset_state()
{
    ending_char = 0;
    level_count = 0;
}

unsigned
tree_sitter_matlab_external_scanner_serialize(void* payload, char* buffer)
{
    buffer[0] = ending_char;
    buffer[1] = level_count;
    return 2;
}

void tree_sitter_matlab_external_scanner_deserialize(void* payload,
    const char* buffer,
    unsigned length)
{
    if (length == 0)
        return;
    ending_char = buffer[0];
    if (length == 1)
        return;
    level_count = buffer[1];
}

bool consume_comment_line(TSLexer* lexer)
{
    while (lexer->lookahead != '\n') {
        consume(lexer);
    }
}

bool scan_comment(TSLexer* lexer)
{
    skip_whitespaces(lexer);

    bool percent = lexer->lookahead == '%';
    bool block = percent && consume_char('%', lexer) && consume_char('{', lexer);
    bool line_continuation = lexer->lookahead == '.' && consume_char('.', lexer) && consume_char('.', lexer) && consume_char('.', lexer);

    if (block) {
        int count = 1000;
        while (count > 0) {
            consume_comment_line(lexer);
            consume(lexer);
            skip_whitespaces(lexer);

            if (consume_char('%', lexer) && consume_char('}', lexer)) {
                lexer->mark_end(lexer);
                lexer->result_symbol = COMMENT;

                return true;
            }

            count--;
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

bool tree_sitter_matlab_external_scanner_scan(void* payload,
    TSLexer* lexer,
    const bool* valid_symbols)
{
    if (valid_symbols[COMMENT]) {
        return scan_comment(lexer);
    }

    return false;
}
