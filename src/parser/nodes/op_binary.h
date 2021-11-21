#ifndef _OP_BINARY_H
#define _OP_BINARY_H

#include "../parser.h"

typedef struct {
    const NodeVTable* vtable;
    TokenRef op;
    NodeRef children[2]; // [lhs, rhs]
    TypeRef type;
} NodeOpBinary;

TypeRef node_op_binary_type(const Parser* parser, NodeOpBinary* op);
NodeRefSlice node_op_binary_children(const Parser* parser, NodeOpBinary* op);
TokenRef node_op_binary_token(const Parser* parser, NodeOpBinary* op);

extern NodeVTable NODE_IMPL_OP_BINARY;

NodeRef node_op_binary_parse(Parser* parser);

#endif