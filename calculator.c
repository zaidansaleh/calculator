#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INPUT_SIZE 1024

#define UNREACHABLE() do { fprintf(stderr, "%s:%d: fatal: unreachable\n", __FILE__, __LINE__); abort(); } while (0)

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

typedef enum {
    AST_NODE_NUMBER,
    AST_NODE_BINARY,
} ASTNodeType;

typedef struct ASTNode {
    ASTNodeType type;
    union {
        struct {
            double value;
        } number;
        struct {
            Token op;
            struct ASTNode *left;
            struct ASTNode *right;
        } binary;
    };
} ASTNode;

ASTNode *ast_node_new() {
    ASTNode *node = malloc(sizeof(ASTNode));
    if (!node) {
        return NULL;
    }
    return node;
}

void ast_node_free(ASTNode *node) {
    if (node->type == AST_NODE_BINARY) {
        ast_node_free(node->binary.left);
        ast_node_free(node->binary.right);
    }
    free(node);
}

void ast_node_pprint(const ASTNode *node, int indent, FILE *stream) {
    for (int i = 0; i < 2 * indent; ++i) {
        fputc(' ', stream);
    }

    switch (node->type) {
    case AST_NODE_NUMBER:
        fprintf(stream, "%g\n", node->number.value);
        break;
    case AST_NODE_BINARY:
        token_println(&node->binary.op, stream);
        ast_node_pprint(node->binary.left, indent + 1, stream);
        ast_node_pprint(node->binary.right, indent + 1, stream);
        break;
    }
}

double ast_node_eval(const ASTNode *node) {
    switch (node->type) {
    case AST_NODE_NUMBER:
        return node->number.value;
    case AST_NODE_BINARY: {
        double left = ast_node_eval(node->binary.left);
        double right = ast_node_eval(node->binary.right);
        switch (node->binary.op.type) {
        case TOKEN_TYPE_PLUS:
            return left + right;
        case TOKEN_TYPE_MINUS:
            return left - right;
        case TOKEN_TYPE_STAR:
            return left * right;
        case TOKEN_TYPE_SLASH:
            return left / right;
        default:
            UNREACHABLE();
        }
    }
    }
    UNREACHABLE();
}

typedef struct {
    Lexer lexer;
    Token current;
    const char *errmsg;
} Parser;

Parser parser_new(const char *input) {
    Lexer lexer = {.pos = input};
    Token current = lexer_next(&lexer);
    Parser parser = {.lexer = lexer, .current = current, .errmsg = NULL};
    return parser;
}

ASTNode *parse_start(Parser *parser);
ASTNode *parse_expr(Parser *parser);
ASTNode *parse_term(Parser *parser);
ASTNode *parse_number(Parser *parser);

ASTNode *parse_start(Parser *parser) {
    ASTNode *root = parse_expr(parser);
    if (parser->current.type != TOKEN_TYPE_EOF) {
        parser->errmsg = "invalid expression";
    }
    return root;
}

ASTNode *parse_expr(Parser *parser) {
    ASTNode *left = parse_term(parser);
    if (!left) {
        return NULL;
    }

    while (parser->current.type == TOKEN_TYPE_PLUS ||
            parser->current.type == TOKEN_TYPE_MINUS) {
        Token op = parser->current;
        parser->current = lexer_next(&parser->lexer);

        ASTNode *right = parse_term(parser);
        if (!right) {
            return left;
        }

        ASTNode *node = ast_node_new();
        if (!node) {
            parser->errmsg = "failed to allocate memory";
            return left;
        }

        node->type = AST_NODE_BINARY;
        node->binary.op = op;
        node->binary.left = left;
        node->binary.right = right;

        left = node;
    }

    return left;
}

ASTNode *parse_term(Parser *parser) {
    ASTNode *left = parse_number(parser);
    if (!left) {
        return NULL;
    }

    while (parser->current.type == TOKEN_TYPE_STAR ||
            parser->current.type == TOKEN_TYPE_SLASH) {
        Token op = parser->current;
        parser->current = lexer_next(&parser->lexer);

        ASTNode *right = parse_number(parser);
        if (!right) {
            return left;
        }

        ASTNode *node = ast_node_new();
        if (!node) {
            parser->errmsg = "failed to allocate memory";
            return left;
        }

        node->type = AST_NODE_BINARY;
        node->binary.op = op;
        node->binary.left = left;
        node->binary.right = right;

        left = node;
    }

    return left;
}

ASTNode *parse_number(Parser *parser) {
    if (parser->current.type != TOKEN_TYPE_NUMBER) {
        parser->errmsg = "expected number";
        return NULL;
    }

    Token token = parser->current;
    parser->current = lexer_next(&parser->lexer);

    char *tmp = malloc(sizeof(char) * (token.length + 1));
    if (!tmp) {
        parser->errmsg = "failed to allocate memory";
        return NULL;
    }

    memcpy(tmp, token.start, token.length);
    tmp[token.length] = '\0';

    char *endptr;
    double value = strtod(tmp, &endptr);
    if (endptr == tmp || *endptr != '\0') {
        parser->errmsg = "invalid number";
        free(tmp);
        return NULL;
    }

    ASTNode *node = ast_node_new();
    if (!node) {
        parser->errmsg = "failed to allocate memory";
        free(tmp);
        return NULL;
    }

    node->type = AST_NODE_NUMBER;
    node->number.value = value;

    free(tmp);
    return node;
}

int main(void) {
    char input[INPUT_SIZE];

    for (;;) {
        printf("> ");

        if (!fgets(input, sizeof(input), stdin)) {
            if (ferror(stdin)) {
                fprintf(stderr, "error: failed to read input: %s\n", strerror(errno));
                return 1;
            } else if (feof(stdin)) {
                fputc('\n', stdout);
                break;
            }
        }

        char *newline = strchr(input, '\n');
        if (!newline) {
            fprintf(stderr, "error: failed to read input: "
                    "input was too long and got cut off (consider increasing the input buffer size)\n");
            return 1;
        }

        Parser parser = parser_new(input);
        ASTNode *root = parse_start(&parser);
        if (parser.errmsg) {
            fprintf(stderr, "error: %s\n", parser.errmsg);
            if (root) {
                ast_node_free(root);
            }
            continue;
        }

        double result = ast_node_eval(root);
        printf("%g\n", result);
        ast_node_free(root);
    }

    return 0;
}

