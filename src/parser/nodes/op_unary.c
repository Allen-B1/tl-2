#include <stdlib.h>
#include "op_unary.h"
#include "grouping.h"
#include "literal.h"

#define CHECK(type_) (parser_getpeek(parser)->type == (type_))

TypeRef node_op_unary_type(const Parser* parser, NodeOpUnary* node) {
    return node->type;
}

NodeRefSlice node_op_unary_children(const Parser* parser, NodeOpUnary* node) {
    return (NodeRefSlice) {
        .data = &node->child,
        .len = 1
    };
}

TokenRef node_op_unary_token(const Parser* parser, NodeOpUnary* node) {
    return node->op;
}


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
NodeVTable NODE_IMPL_OP_UNARY = {
    .name = "OpUnary",
    .children = node_op_unary_children,
    .token = node_op_unary_token,
    .type = node_op_unary_type
};
#pragma GCC diagnostic pop

#define IS_OP_UNARY(type) ((type) == TOKEN_ADD || (type) == TOKEN_SUB || (type) == TOKEN_MUL || (type) == TOKEN_BIT_NOT || (type) == TOKEN_BOOL_NOT || (type) == TOKEN_BIT_AND || (type) == TOKEN_QUESTION)
NodeRef node_op_unary_parse(Parser* parser) {
	if (!IS_OP_UNARY(parser_getpeek(parser)->type) && !CHECK(TOKEN_BRACKET_LEFT)) return node_grouping_parse(parser); //node_func_call_parse(parser);

	TokenRef op_ref = parser_consume(parser);
    Token* op = parser_gettok(parser, op_ref);
	uint64_t data = UINT64_MAX;

	if (op->type == TOKEN_BRACKET_LEFT) {
		if (!CHECK(TOKEN_BRACKET_RIGHT)) {
			TokenRef num_ref = parser_consume(parser);
            Token* num = parser_gettok(parser, num_ref);
			if (num->type != TOKEN_LIT_INT) {
				RET_ERROR(parser, "expected integer literal or ]");
			}

			data = num_ref;
		}
	}

	if ((op->type == TOKEN_BRACKET_LEFT || op->type == TOKEN_MUL) && data == 0) {
		if (CHECK(TOKEN_MUT)) {
			parser_consume(parser);
			data = TYPE_MUT;
		}
	}

    NodeRef child = node_op_unary_parse(parser);
    RET_IF_ERR(parser, child);

    NodeOpUnary* node = malloc(sizeof(NodeOpUnary));
    RET_IF_OOM(parser, node);
    node->vtable = &NODE_IMPL_OP_UNARY;
    node->op = op_ref;
    node->child = child;
    node->data = data;
    
    Node* child_node = arrlist_get(&parser->nodes, child);
    TypeRef child_typeref = child_node->vtable->type(parser, child_node);
    TypeEntry* child_typeentry = typetable_get(&parser->types, child_typeref);
    Type* child_type = &child_typeentry->type;

	switch (child_type->tag) {
	case TYPE_INT:
	case TYPE_UINT:
	case TYPE_FLOAT:
		if (op->type != TOKEN_ADD && op->type != TOKEN_SUB && op->type != TOKEN_BIT_NOT && op->type != TOKEN_BIT_AND) {
			RET_ERROR(parser, "invalid number type for unary operator");
		}
		break;
	case TYPE_BOOL:
		if (op->type != TOKEN_BOOL_NOT && op->type != TOKEN_BIT_AND) {
			RET_ERROR(parser, "invalid type bool for unary operator");
		}
		break;
	case TYPE_TYPE:
		if (op->type != TOKEN_QUESTION && op->type != TOKEN_BRACKET_LEFT && op->type != TOKEN_MUL) {
			RET_ERROR(parser, "invalid type type for unary operator");
		}

/* TODO		TypeRef inner = eval_type(parser, rhs);
		if (!type_is_runtime(inner)) {
			RET_ERROR(parser, "must have runtime inner type");
		}

		if (op->type == TOKEN_QUESTION) {
			if (inner->type.tag != TYPE_SLICE && inner->type.tag != TYPE_PTR) {
				RET_ERROR("optional must be pointer or slice type");
			}
		}*/

		break;
    case TYPE_PTR:
        if (op->type != TOKEN_MUL && op->type != TOKEN_BIT_AND) {
            RET_ERROR(parser, "pointer type can only work with deref unary operator");
        }
        break;
	default:
		if (op->type != TOKEN_BIT_AND) 
			RET_ERROR(parser, "invalid type for unary operator");
		break;
	}

    if (op->type == TOKEN_MUL) {
		if (child_type->tag == TYPE_TYPE) {
			node->type = TYPEREF_TYPE;
		} else if (child_type->tag != TYPE_PTR) {
            // unreachable!()
            fprintf(stderr, "unreachable code reached: dereference of non-pointer value");
            abort();
        } else {
			// dereference
			node->type = child_type->child;
		}
    } else if (op->type == TOKEN_BIT_AND) {
        Type type = {
            .child = child_typeref,
            .data = TYPE_MUT,
            .tag= TYPE_PTR
        };
        node->type = typetable_add(&parser->types, "", type);
    } else {
        node->type = child_typeref;
    }

    return parser_addnode(parser, (Node*)node);
}
