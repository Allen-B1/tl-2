#ifndef _OP_UNARY_H
#define _OP_UNARY_H

#include "../parser.h"

typedef struct {
    const NodeVTable* vtable;
    TokenRef op;
    NodeRef child;
    TypeRef type;
    uint64_t data;
} NodeOpUnary;

TypeRef node_op_unary_type(const Parser* parser, NodeOpUnary* node);
NodeRefSlice node_op_unary_children(const Parser* parser, NodeOpUnary* node);
TokenRef node_op_unary_token(const Parser* parser, NodeOpUnary* node);
NodeRef node_op_unary_parse(Parser* parser);

extern NodeVTable NODE_IMPL_OP_UNARY;

#endif