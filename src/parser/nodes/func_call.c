#include "func_call.h"
#include "../types.h"
#include "grouping.h"
#include "op_binary.h"
#include <assert.h>

TypeRef node_func_call_type(const Parser* parser, NodeFuncCall* node) {
    Node* n = arrlist_get(&parser->nodes, node->children[0]);

    assert(n != NULL);
    assert(n->vtable->type != NULL);

    TypeRef func_typeref = n->vtable->type(parser, n);
    TypeEntry* func_type = typetable_get(&parser->types, func_typeref);

    assert (func_type->type.tag == TYPE_FUNC);

    TypeFuncData* func = (TypeFuncData*)func_type->type.data;
    return func->ret_type;
}

TokenRef node_func_call_token(const Parser* parser, NodeFuncCall* node) {
    return node->paren_left;
}

NodeRefSlice node_func_call_children(const Parser* parser, NodeFuncCall* node) {
    return (NodeRefSlice){
        .len = node->children_len,
        .data = &node->children[0]
    };
}


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
NodeVTable NODE_IMPL_FUNC_CALL = {
    .children = node_func_call_children,
    .token = node_func_call_token,
    .type = node_func_call_type,
    .name = "NodeFuncCall"
};
#pragma GCC diagnostic pop


#define CHECK(type_) (parser_getpeek(parser)->type == (type_))
NodeRef node_func_call_parse(Parser* parser) {
    NodeRef func = node_grouping_parse(parser);
    RET_IF_ERR(parser, func);

	if (!CHECK(TOKEN_PAREN_LEFT)) {
		return func;
	}

	TokenRef left_ref = parser_consume(parser);

    Node* func_node = arrlist_get(&parser->nodes, func);
	TypeRef func_typeref = func_node->vtable->type(parser, func_node);
    TypeEntry* func_type = typetable_get(&parser->types, func_typeref);

	if (func_type->type.tag != TYPE_FUNC) {
		RET_ERROR(parser, "lhs of function call is not a function");
	}

	TypeFuncData* func_data = (void*)func_type->type.data;

	NodeFuncCall* call = malloc(sizeof(NodeFuncCall) + 2 * sizeof(NodeRef));
	RET_IF_OOM(parser, call);
    call->vtable = &NODE_IMPL_FUNC_CALL;
    call->paren_left = left_ref;
    call->children[0] = func;
    call->children_len = 1;

	if (CHECK(TOKEN_PAREN_RIGHT)) {
		if (func_data->arg_types.len != 0) {
			RET_ERROR(parser, "function call has wrong # of args");
		}

		call->paren_right = parser_consume(parser);
        return arrlist_add(&parser->nodes, call);
	}

	size_t cap = 1;
	for (;;) {
		NodeRef arg = node_op_binary_parse(parser);
		RET_IF_ERR(parser, arg);

        Node* arg_node = arrlist_get(&parser->nodes, arg);
        TypeRef arg_typeref = arg_node->vtable->type(parser, arg_node);

		if (func_data->arg_types.len <= call->children_len - 1) {
			if (!func_data->varardic) {
				RET_ERROR(parser, "function call has too many arguments");
			}
		} else {
			if (!type_can_coerce(&parser->types, arg_typeref, (TypeRef)arrlist_get(&func_data->arg_types, call->children_len - 1))) {
				RET_ERROR(parser, "argument has incompatible type");
			}
		}

		if (cap < call->children_len + 1) {
			cap *= 2;
			call = realloc(call, sizeof(NodeFuncCall) + sizeof(NodeRef) * cap);
            RET_IF_OOM(parser, call);
		}

        call->children[call->children_len] = arg;
        call->children_len += 1;
	}

	if (func_data->arg_types.len > call->children_len - 1) RET_ERROR(parser, "function call has too few arguments");

    return arrlist_add(&parser->nodes, call);
}