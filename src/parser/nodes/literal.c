#include "../parser.h"
#include "literal.h"

TokenRef node_literal_token(const Parser* parser, NodeLiteral* literal) {
    return literal->token;
}

TypeRef node_literal_type(const Parser* parser, NodeLiteral* literal) {
    Token* token = arrlist_get(&parser->tokens, literal->token);
    switch (token->type) {
	case TOKEN_LIT_INT: return TYPEREF_GENERIC_INT;
	case TOKEN_LIT_FLOAT: return TYPEREF_GENERIC_FLOAT;
	case TOKEN_LIT_STR: return TYPEREF_STR;
    default: return TYPEREF_ERR;
    }
}

NodeRef node_literal_parse(Parser* parser) {
    TokenRef tokref = parser_consume(parser);
    Token* tok = parser_gettok(parser, tokref);
    if (tok == NULL) {
        RET_ERROR(parser, "expected int, float, or string literal, found EOF");
    }
	if (tok->type != TOKEN_LIT_INT && tok->type != TOKEN_LIT_FLOAT && tok->type != TOKEN_LIT_STR) {
		RET_ERROR(parser,"expected int, float, or string literal");
	}

    NodeLiteral* out = malloc(sizeof(NodeLiteral));
    RET_IF_OOM(parser, out);
    out->vtable = &NODE_IMPL_LITERAL;
    out->token = tokref;
    return parser_addnode(parser, (Node*)out);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
NodeVTable NODE_IMPL_LITERAL = {
    .name = "Literal",
    .type = (void*)node_literal_type,
    .children = NULL,
    .token = (void*)node_literal_token,
};
#pragma GCC diagnostic pop