#include "ident.h"
TypeRef node_ident_type(const Parser* parser, NodeIdent* ident) {
    // TODO: symbol table
    return TYPEREF_TYPE;
}

TokenRef node_ident_token(const Parser* parser, NodeIdent* ident) {
    return ident->first_token;
}

NodeRef node_ident_parse(Parser* parser) {
    TokenRef tokref;
    if(!parser_consume_if(parser, TOKEN_IDENT, &tokref)) {
        RET_ERROR(parser,"expected identifier");
    }
    Token* token = parser_gettok(parser, tokref);

	char* str = malloc(token->len+1);
	RET_IF_OOM(parser,str);

	size_t len = 0;
	memcpy(&str[len], token->start, token->len);
	len += token->len;

    TokenRef lastref = tokref;
	for (;;) {
		if (parser_peek_is(parser, TOKEN_COLONS)) {
			parser_consume(parser);

			str = realloc(str, len+2);			
			RET_IF_OOM(parser,str);
	
			memcpy(&str[len], "::", 2);
			len += 2;
		} else { break; }

        if (!parser_consume_if(parser, TOKEN_IDENT, &lastref)) {
            RET_ERROR(parser,"expected identifier");
        }
        Token* ident = parser_gettok(parser, lastref);

		str = realloc(str, len + ident->len + 1);
		RET_IF_OOM(parser,str);

		memcpy(&str[len], ident->start, ident->len);
		len += ident->len;
	}

	str[len] = '\0';

    NodeIdent* out = malloc(sizeof(NodeIdent));
    RET_IF_OOM(parser,out);
    out->vtable = &NODE_IMPL_IDENT;
    out->first_token = tokref;
    out->last_token = lastref;
    out->name = str;
    return parser_addnode(parser, (Node*)out);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
NodeVTable NODE_IMPL_IDENT = {
    .name = "Ident",
    .token = node_ident_token,
    .type = node_ident_type,
    .children = NULL
}; 
#pragma GCC diagnostic pop
