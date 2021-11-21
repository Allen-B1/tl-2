#include "op_binary.h"
#include "literal.h"

TypeRef node_op_binary_type(const Parser* parser, NodeOpBinary* op) {
    return op->type;
}

NodeRefSlice node_op_binary_children(const Parser* parser, NodeOpBinary* op) {
    NodeRefSlice slice = {.len=2, .data=op->children};
    return slice;
}

TokenRef node_op_binary_token(const Parser* parser, NodeOpBinary* op) {
    return op->op;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
NodeVTable NODE_IMPL_OP_BINARY = {
    .name = "OpBinary",
    .type = node_op_binary_type,
    .children = node_op_binary_children,
    .token = node_op_binary_token
};
#pragma GCC diagnostic pop

#define DEFINE_OP_LEFT(name, parse_stronger, test_op, rt_func, err_str) \
	static NodeRef parse_##name(Parser* parser) { \
		NodeRef lhs = parse_stronger(parser); \
		RET_IF_ERR(parser,lhs); \
\
		for (;;) { \
			if (!test_op(parser_getpeek(parser)->type)) return lhs; \
			TokenRef op = parser_consume(parser); \
\
			NodeRef rhs = parse_stronger(parser); \
			RET_IF_ERR(parser,rhs); \
\
			NodeOpBinary* out = malloc(sizeof(NodeOpBinary)); \
			out->vtable = &NODE_IMPL_OP_BINARY; \
			out->op = op; \
\
			Node* lhs_node = arrlist_get(&parser->nodes, lhs); \
			Node* rhs_node = arrlist_get(&parser->nodes, rhs); \
			if (lhs_node->vtable->type == NULL || rhs_node->vtable->type == NULL) { \
				RET_ERROR(parser, "expected expression, found statement in binary op"); \
			} \
\
			TypeRef ret_type = rt_func(parser, lhs_node->vtable->type(parser, lhs_node), rhs_node->vtable->type(parser, rhs_node)); \
			if (ret_type == TYPEREF_ERR) { \
                RET_ERROR(parser,err_str); \
			} \
            out->type = ret_type; \
            out->children[0] = lhs; \
            out->children[1] = rhs; \
\
			lhs = parser_addnode(parser, (Node*)out); \
		}\
	}

#define IS_OP_MUL(type) ((type) == TOKEN_MUL || (type) == TOKEN_DIV || (type) == TOKEN_MOD || (type) == TOKEN_SHIFT_LEFT || (type) == TOKEN_SHIFT_RIGHT || (type) == TOKEN_BIT_AND)
#define IS_OP_ADD(type) ((type) == TOKEN_ADD || (type) == TOKEN_SUB || (type) == TOKEN_BIT_OR || (type) == TOKEN_BIT_XOR)
#define IS_OP_CMP(type) ((type) == TOKEN_CMP_EQ || (type) == TOKEN_CMP_NE || (type) == TOKEN_CMP_LE || (type) == TOKEN_CMP_GE || (type) == TOKEN_CMP_LT || (type) == TOKEN_CMP_GT)
#define IS_OP_AND(type) ((type) == TOKEN_BOOL_AND)
#define IS_OP_OR(type) ((type) == TOKEN_BOOL_OR)

static inline TypeRef rt_op(Parser* parser, TypeRef lhs, TypeRef rhs) {
	if (!type_can_coerce(&parser->types ,rhs, lhs)) {
		return TYPEREF_ERR;
	}
	return lhs;
}

inline static TypeRef rt_cmp(Parser* parser, TypeRef lhs, TypeRef rhs) {
	if (!type_can_coerce(&parser->types, rhs, lhs)) {
		return TYPEREF_ERR;
	}
	return TYPEREF_BOOL;
}

inline static  TypeRef rt_bool(Parser* parser, TypeRef lhs, TypeRef rhs) {
	if (typetable_get(&parser->types, lhs)->type.tag != TYPE_BOOL || typetable_get(&parser->types, rhs)->type.tag != TYPE_BOOL) return TYPEREF_ERR;
	return TYPEREF_BOOL;
}

DEFINE_OP_LEFT(op_mul, /*TODO node_grouping_parse*/ node_literal_parse, IS_OP_MUL, rt_op, "incompatible lhs and rhs types")
DEFINE_OP_LEFT(op_add, parse_op_mul, IS_OP_ADD, rt_op, "incompatible lhs and rhs types")
DEFINE_OP_LEFT(op_cmp, parse_op_add, IS_OP_CMP, rt_cmp, "incompatible lhs and rhs types")
DEFINE_OP_LEFT(op_and, parse_op_cmp, IS_OP_AND, rt_bool, "lhs and rhs must be bool")
DEFINE_OP_LEFT(op_or, parse_op_and, IS_OP_OR, rt_bool, "lhs and rhs must be bool")

NodeRef node_op_binary_parse(Parser* parser) {
    return parse_op_or(parser);
}
