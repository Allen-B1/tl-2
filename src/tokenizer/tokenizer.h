#ifndef _TOKENIZER_H
#define _TOKENIZER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
	const char* start;
	const char* current;
	int line;
} Tokenizer;

typedef enum {
	TOKEN_ERR_UNEXPECTED,
	TOKEN_ERR_FLOAT_REQUIRE_EXP,
	TOKEN_ERR_STRING_REQUIRE_TERMINATION,
	TOKEN_ERR_COMMENT_REQUIRE_TERMINATION,
	TOKEN_ERR_GENERIC,

	TOKEN_EOF,
	TOKEN_LIT_STR,
	TOKEN_LIT_INT,
	TOKEN_LIT_FLOAT,
	TOKEN_IDENT,

	TOKEN_PAREN_LEFT,
	TOKEN_PAREN_RIGHT,
	TOKEN_BRACE_LEFT,
	TOKEN_BRACE_RIGHT,
	TOKEN_BRACKET_LEFT, 
	TOKEN_BRACKET_RIGHT,

	TOKEN_COMMA,
	TOKEN_SEMICOLON,
	TOKEN_DOT,
	TOKEN_COLONS,
	TOKEN_COLON,

	TOKEN_ADD, TOKEN_SUB, TOKEN_MUL, TOKEN_DIV, TOKEN_MOD,
	TOKEN_BIT_AND, TOKEN_BIT_OR, TOKEN_BIT_XOR, TOKEN_BIT_NOT, TOKEN_SHIFT_LEFT, TOKEN_SHIFT_RIGHT,
	TOKEN_CMP_EQ, TOKEN_CMP_LT, TOKEN_CMP_GT, TOKEN_CMP_LE, TOKEN_CMP_GE, TOKEN_CMP_NE,
	TOKEN_BOOL_OR, TOKEN_BOOL_AND, TOKEN_BOOL_NOT,
	TOKEN_EQ,
		TOKEN_EQ_ADD, TOKEN_EQ_SUB, TOKEN_EQ_MUL, TOKEN_EQ_DIV, TOKEN_EQ_MOD,
		TOKEN_EQ_BIT_AND, TOKEN_EQ_BIT_OR, TOKEN_EQ_BIT_XOR, TOKEN_EQ_SHIFT_LEFT, TOKEN_EQ_SHIFT_RIGHT,
	TOKEN_QUESTION,

	TOKEN_COMMENT, TOKEN_COMMENT_MULTI,

	TOKEN_STRUCT, TOKEN_UNION, TOKEN_ENUM,
	TOKEN_STATIC, TOKEN_LET, TOKEN_MUT, TOKEN_CONST, TOKEN_TYPE, TOKEN_FUNC, TOKEN_RETURN,
	TOKEN_IF, TOKEN_ELSE, TOKEN_SWITCH, TOKEN_CASE, TOKEN_FOR, TOKEN_BREAK, TOKEN_CONTINUE
} TokenType;

typedef struct {
	TokenType type;
	const char* start;
	size_t len;
	int line;
} Token;


void tok_init(Tokenizer* tokenizer, const char* src);
Token tok_next(Tokenizer* tokenizer);

#endif

