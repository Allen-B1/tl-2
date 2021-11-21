#ifndef _LITERAL_H
#define _LITERAL_H

#include "../parser.h"

#include "../../tokenizer/tokenizer.h"

typedef struct {
    const NodeVTable* vtable;
    TokenRef token;
} NodeLiteral;

TypeRef node_literal_type(const Parser*, NodeLiteral*);
TokenRef node_literal_token(const Parser*, NodeLiteral*);

extern NodeVTable NODE_IMPL_LITERAL;

NodeRef node_literal_parse(Parser*);

#endif