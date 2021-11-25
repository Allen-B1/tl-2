#ifndef _PARSER_H
#define _PARSER_H

#include "parser_forward.h"
#include "node.h"
#include "symbols.h"

#include "../tokenizer/tokenizer.h"
#include "types.h"
#include <arrlist.h>

struct Parser {
	Tokenizer tok;

	const char* error;
//	ArrList errors;

	// last element is next token to be consumed
	ArrList tokens;

	ArrList nodes;

	TypeTable types;

	// maximum 256 depth
	SymbolTable scopes[256];
	size_t current_scope;
};

static void parser_init(Parser* parser, const char* src) {
	tok_init(&parser->tok, src);
	parser->error = NULL;
	arrlist_init(&parser->tokens, 32);
	arrlist_init(&parser->nodes, 32);
	typetable_init(&parser->types);
	symbols_init(&parser->scopes[0]);
	symbols_add_builtin(&parser->scopes[0], &parser->types);
	parser->current_scope = 0;

	Token tok = tok_next(&parser->tok);
	Token* tok_m = malloc(sizeof(Token));
	memcpy(tok_m, &tok, sizeof(Token));
	arrlist_add(&parser->tokens, tok_m);
}

static TokenRef parser_consume(Parser* parser) {
	Token tok = tok_next(&parser->tok);
	Token* tok_m = malloc(sizeof(Token));
	if (tok_m == NULL) {
		fprintf(stderr, "parser_consume: out of memory\n");
		abort();
	}
	memcpy(tok_m, &tok, sizeof(Token));

	TokenRef ref = parser->tokens.len - 1;
	arrlist_add(&parser->tokens, tok_m);
	return ref;
}

static inline Token* parser_gettok(Parser* parser, TokenRef ref) {
	return arrlist_get(&parser->tokens, ref);
}

static inline TokenRef parser_peek(Parser* parser) {
	return parser->tokens.len - 1;
}

static inline bool parser_peek_is(Parser* parser, TokenType type) {
	return parser_gettok(parser, parser->tokens.len - 1)->type == type;
}

static inline bool parser_consume_if(Parser* parser, TokenType type, TokenRef* out) {
	if (((Token*)arrlist_get(&parser->tokens, parser_peek(parser)))->type == type) {
		*out = parser_consume(parser);
		return true;
	} else {
		return false;
	}
}

static inline Token* parser_getpeek(Parser* parser) {
	return parser_gettok(parser, parser_peek(parser));
}

#define RET_IF_OOM(parser, expr) do { \
		if ((expr) == NULL) { parser->error = "out of memory"; return NODE_ERR; } \
	} while(0)

#define RET_ERROR(parser, err) do { \
		parser->error = err; \
		return NODE_ERR; \
	} while(0)

#define RET_IF_ERR(parser, node) do { \
		if ((node) == NODE_ERR) return NODE_ERR; \
	} while (0); 

#define PARSER_ERR(parser, err) do { \
	parser->error = err; } while(0)


static inline NodeRef parser_addnode(Parser* parser, Node* node) {
	arrlist_add(&parser->nodes, node);
	return parser->nodes.len - 1;
}

#endif