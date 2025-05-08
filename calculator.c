#include <ctype.h>
#include <stdio.h>

typedef enum {
    TOKEN_TYPE_NUMBER,
    TOKEN_TYPE_PLUS,
    TOKEN_TYPE_MINUS,
    TOKEN_TYPE_STAR,
    TOKEN_TYPE_SLASH,
    TOKEN_TYPE_INVALID,
    TOKEN_TYPE_EOF,
} TokenType;

typedef struct {
    TokenType type;
    const char *start;
    size_t length;
} Token;

void token_println(const Token *token, FILE *stream) {
    fprintf(stream, "%.*s\n", (int)token->length, token->start);
}

typedef struct {
    const char *pos;
} Lexer;

Token lexer_next(Lexer *lexer) {
    while (isspace(*lexer->pos)) {
        lexer->pos++;
    }

    Token token;

    if (*lexer->pos == '\0') {
        token.type = TOKEN_TYPE_EOF;
        return token;
    }

    switch (*lexer->pos) {
    case '+':
        token.type = TOKEN_TYPE_PLUS;
        token.start = lexer->pos++;
        token.length = 1;
        return token;
    case '-':
        token.type = TOKEN_TYPE_MINUS;
        token.start = lexer->pos++;
        token.length = 1;
        return token;
    case '*':
        token.type = TOKEN_TYPE_STAR;
        token.start = lexer->pos++;
        token.length = 1;
        return token;
    case '/':
        token.type = TOKEN_TYPE_SLASH;
        token.start = lexer->pos++;
        token.length = 1;
        return token;
    }

    if (*lexer->pos >= '1' && *lexer->pos <= '9') {
        const char *start = lexer->pos++;
        while (*lexer->pos >= '0' && *lexer->pos <= '9') {
            lexer->pos++;
        }
        token.type = TOKEN_TYPE_NUMBER;
        token.start = start;
        token.length = lexer->pos - start;
        return token;
    }

    token.type = TOKEN_TYPE_INVALID;
    return token;
}

int main(void) {
    const char *input = "1 + 2 - 3 * 4 / 5";

    Lexer lexer = {.pos = input};
    Token token;
    while ((token = lexer_next(&lexer)).type != TOKEN_TYPE_EOF) {
        token_println(&token, stdout);
    }

    return 0;
}

