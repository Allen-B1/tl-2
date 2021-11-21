#ifndef _PARSER_FORWARD_H
#define _PARSER_FORWARD_H

#include "../tokenizer/tokenizer.h"
#include <stdbool.h>
#include <stddef.h>

typedef size_t TokenRef;

typedef size_t NodeRef;

typedef struct {
    size_t len;
    NodeRef* data;
} NodeRefSlice;


typedef struct Parser Parser;
static void parser_init(Parser* parser, const char* src);
static TokenRef parser_consume(Parser* parser);
static inline Token* parser_gettok(Parser* parser, TokenRef ref);
static inline TokenRef parser_peek(Parser* parser);
static inline bool parser_peek_is(Parser* parser, TokenType type);
static inline bool parser_consume_if(Parser* parser, TokenType type, TokenRef* out);
static inline Token* parser_getpeek(Parser* parser);
#endif