#ifndef _IDENT_H
#define _IDENT_H

#include "../parser.h"

typedef struct {
    const NodeVTable* vtable;
    TokenRef first_token;
    TokenRef last_token;
    size_t scope;
    char* name;
} NodeIdent;

TypeRef node_ident_type(const Parser* parser, NodeIdent* ident);
TokenRef node_ident_token(const Parser* parser, NodeIdent* ident);

extern NodeVTable NODE_IMPL_IDENT;

char* ident_parse(Parser* parser, TokenRef* start, TokenRef* end);
NodeRef node_ident_parse(Parser* parser);

#endif