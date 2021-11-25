#ifndef _FUNC_CALL_H
#define _FUNC_CALL_H

#include "../parser.h"

typedef struct {
    const NodeVTable* vtable;

    TokenRef paren_left;
    TokenRef paren_right;

    size_t children_len;
    NodeRef children[];
} NodeFuncCall;

TypeRef node_func_call_type(const Parser* parser, NodeFuncCall* node);
TokenRef node_func_call_token(const Parser* parser, NodeFuncCall* node);
NodeRefSlice node_func_call_children(const Parser* parser, NodeFuncCall* node);

extern NodeVTable NODE_IMPL_FUNC_CALL;

NodeRef node_func_call_parse(Parser* parser);

#endif