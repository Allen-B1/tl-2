#ifndef _LET_H
#define _LET_H

#include "../parser.h"
#include "../../tokenizer/tokenizer.h"

typedef struct {
    const NodeVTable* vtable;

    TokenRef kwd;

    TokenRef mut; // may or may not exist; TOKREF_ERR if not
    TokenRef linkage; // may or may not exist; TOKREF_ERR if not

    TokenRef ident_start;
    TokenRef ident_end;
    const char* ident_name;

    NodeRef type;
    NodeRef value;
} NodeLet;

TokenRef node_let_token(const Parser*, NodeLet*);
NodeRefSlice node_let_children(const Parser*, NodeLet*);

extern NodeVTable NODE_IMPL_LET;

NodeRef node_let_parse(Parser*, TokenRef visiblity);

#endif