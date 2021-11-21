#include <ctype.h>
#include <stdint.h>
#include <stdbool.h>

#include "tokenizer.h"

void tok_init(Tokenizer* tokenizer, const char* src) {
	tokenizer->start = src;
	tokenizer->current = src;
	tokenizer->line = 0;
}

// increments current and returns the
// previous value at current.
static char tok_consume(Tokenizer* tokenizer) {
	tokenizer->current++;
	if (tokenizer->current[-1] == '\n') tokenizer->line++;
	return tokenizer->current[-1];
}

// if tokenizer->current[0] is c, return true
// and consume. ot
static bool tok_match(Tokenizer* tokenizer, char c) {
	if (tokenizer->current[0] == c) {
		tokenizer->current++;
		return true;
	}
	return false;
}

#define MAKE_TOKEN(typ) (Token){.type=(typ),.start=tokenizer->start, .len=tokenizer->current - tokenizer->start, .line=tokenizer->line}

static Token tok_number(Tokenizer* tokenizer) {
	bool is_e = false;
	bool is_dot = false;
	for (;;) {
		char nxt = *tokenizer->current;

		switch (nxt) {
		case 'e':
			if (is_e) goto end;

			is_e = true;
			is_dot = false;
			break;
		case '.':
			if (is_dot) goto end;
			is_dot = true;
			break;
		case '0'...'9': break;
		default: 
			goto end;
		}

		tok_consume(tokenizer);
	}
	end:

	if (tokenizer->current[-1] == 'e') {
		return MAKE_TOKEN(TOKEN_ERR_FLOAT_REQUIRE_EXP);
	}

	return MAKE_TOKEN(is_e || is_dot ? TOKEN_LIT_FLOAT : TOKEN_LIT_INT);
}

static Token tok_string(Tokenizer* tokenizer) {
	tok_consume(tokenizer); // assume "

	while (tokenizer->current[0] != '"') {
		if (tokenizer->current[0] == '\0') return MAKE_TOKEN(TOKEN_ERR_STRING_REQUIRE_TERMINATION);
		char c = tok_consume(tokenizer);
		if (c == '\\') {
			tok_consume(tokenizer);
		}
	}

	tok_consume(tokenizer);

	return MAKE_TOKEN(TOKEN_LIT_STR);
}


static const char* const keywords[] = {
	"struct",
	"union",
	"enum",

	"let",
	"mut",
	"const",
	"type",
	"func", "return",

	"if", "else",
	"switch", "case",
	"for", "break", "continue"};
static const TokenType keyword_tokens[] = {
	TOKEN_STRUCT, TOKEN_UNION, TOKEN_ENUM,
	TOKEN_LET, TOKEN_MUT, TOKEN_CONST, TOKEN_TYPE, TOKEN_FUNC, TOKEN_RETURN,
	TOKEN_IF, TOKEN_ELSE, TOKEN_SWITCH, TOKEN_CASE, TOKEN_FOR, TOKEN_BREAK, TOKEN_CONTINUE
};

static bool tok_keyword_match(const char* keyword, const char* src, const char* current) {
	while (*keyword != '\0') {
		if (*(keyword++) != *(src++)) return false;
	}
	return src == current;
}

static Token tok_ident(Tokenizer* tokenizer) {
	while (isalnum(tokenizer->current[0]) || tokenizer->current[0] == '_') {
		tok_consume(tokenizer);
	}

	for (int i = 0; i < sizeof (keywords) / sizeof (*keywords); i++) {
		if (tok_keyword_match(keywords[i], tokenizer->start, tokenizer->current)) {
			return MAKE_TOKEN(keyword_tokens[i]);
		}
	}

	return MAKE_TOKEN(TOKEN_IDENT);
}

static void tok_skip_whitespace(Tokenizer* tokenizer) {
	while (isspace(tokenizer->current[0])) tok_consume(tokenizer);
}

static Token tok_comment(Tokenizer* tokenizer) {
	char c = tok_consume(tokenizer);
	if (c == '/') {
		while (tokenizer->current[0] != '\n' && tokenizer->current[0] != '\0') tok_consume(tokenizer);
		return MAKE_TOKEN(TOKEN_COMMENT);
	} else {
		while (!(tokenizer->current[0] == '*' && tokenizer->current[1] == '/')) {
			if (tokenizer->current[0] == '\0') return MAKE_TOKEN(TOKEN_ERR_COMMENT_REQUIRE_TERMINATION);
			tok_consume(tokenizer);
		}
		tok_consume(tokenizer);
		tok_consume(tokenizer);
		return MAKE_TOKEN(TOKEN_COMMENT_MULTI);
	}
}

Token tok_next(Tokenizer* tokenizer) {
	tok_skip_whitespace(tokenizer);

	tokenizer->start = tokenizer->current;

	char next = tok_consume(tokenizer);
	switch (next) {
	#define MATCH(...) tok_match(tokenizer, __VA_ARGS__)
	case '\0': return MAKE_TOKEN(TOKEN_EOF);
		
	case '{': return MAKE_TOKEN(TOKEN_BRACE_LEFT);
	case '}': return MAKE_TOKEN(TOKEN_BRACE_RIGHT);
	case '[': return MAKE_TOKEN(TOKEN_BRACKET_LEFT);
	case ']': return MAKE_TOKEN(TOKEN_BRACKET_RIGHT);
	case '(': return MAKE_TOKEN(TOKEN_PAREN_LEFT);
	case ')': return MAKE_TOKEN(TOKEN_PAREN_RIGHT);

	case ',': return MAKE_TOKEN(TOKEN_COMMA);
	case ';': return MAKE_TOKEN(TOKEN_SEMICOLON);
	case '.': return MAKE_TOKEN(TOKEN_DOT); 
	case ':': return MAKE_TOKEN(MATCH(':') ? TOKEN_COLONS : TOKEN_COLON);

	#define OP(name) (tok_match(tokenizer, '=') ? TOKEN_EQ_##name : TOKEN_##name)
	#define MAKE_OP(name) MAKE_TOKEN(tok_match(tokenizer, '=') ? TOKEN_EQ_##name : TOKEN_##name)
	case '+': return MAKE_OP(ADD);
	case '-': return MAKE_OP(SUB);
	case '*': return MAKE_OP(MUL);
	case '/': {
		if (*tokenizer->current == '/' || *tokenizer->current == '*') return tok_comment(tokenizer);
		return MAKE_OP(DIV);
	}
	case '%': return MAKE_OP(MOD);
	case '&': return MAKE_TOKEN(MATCH('&') ? TOKEN_BOOL_AND : OP(BIT_AND));
	case '|': return MAKE_TOKEN(MATCH('|') ? TOKEN_BOOL_OR : OP(BIT_OR));
	case '^': return MAKE_OP(BIT_OR);
	case '~': return MAKE_TOKEN(TOKEN_BIT_NOT);
	case '!': return MAKE_TOKEN(MATCH('=') ? TOKEN_CMP_NE : TOKEN_BOOL_NOT);
	case '=': return MAKE_TOKEN(MATCH('=') ? TOKEN_CMP_EQ : TOKEN_EQ);
	case '<': return MAKE_TOKEN(
		MATCH('<') ? OP(SHIFT_LEFT) :
		MATCH('=') ? TOKEN_CMP_LE : TOKEN_CMP_LT);
	case '>': return MAKE_TOKEN(
		MATCH('>') ? OP(SHIFT_RIGHT) :
		MATCH('=') ? TOKEN_CMP_GE : TOKEN_CMP_GT);
	case '?': return MAKE_TOKEN(TOKEN_QUESTION);

	case '0'...'9': return tok_number(tokenizer);
	case '"': return tok_string(tokenizer);
	case 'A'...'Z':
	case 'a'...'z':
	case '_':
		return tok_ident(tokenizer);
	default:
		return MAKE_TOKEN(TOKEN_ERR_UNEXPECTED);
	}
	#undef MATCH
	#undef OP
	#undef MAKE_OP
}

#undef MAKE_TOKEN
