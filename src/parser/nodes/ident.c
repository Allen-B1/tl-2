#include "ident.h"
#include <assert.h>

TypeRef node_ident_type(const Parser* parser, NodeIdent* ident) {
    SymbolEntry* entry = symbols_get(&parser->scopes[ident->scope], ident->name);
    assert (entry != NULL);
    return entry->type;
}

TokenRef node_ident_token(const Parser* parser, NodeIdent* ident) {
    return ident->first_token;
}

char* ident_parse(Parser* parser, TokenRef* start, TokenRef* end) {
    TokenRef tokref;
    if(!parser_consume_if(parser, TOKEN_IDENT, &tokref)) {
        PARSER_ERR(parser,"expected identifier");
        return NULL;
    }
    *start = tokref;
    Token* token = parser_gettok(parser, tokref);

	char* str = malloc(token->len+1);
    if (str == NULL) {
        PARSER_ERR(parser, "out of memory");
        return NULL;
    }

	size_t len = 0;
	memcpy(&str[len], token->start, token->len);
	len += token->len;

    TokenRef lastref = tokref;
	for (;;) {
		if (parser_peek_is(parser, TOKEN_COLONS)) {
			parser_consume(parser);

			str = realloc(str, len+2);			
            if (str == NULL) {
                PARSER_ERR(parser, "out of memory");
                return NULL;
            }
	
			memcpy(&str[len], "::", 2);
			len += 2;
		} else { break; }

        if (!parser_consume_if(parser, TOKEN_IDENT, &lastref)) {
            PARSER_ERR(parser,"expected identifier");
            return NULL;
        }
        Token* ident = parser_gettok(parser, lastref);

		str = realloc(str, len + ident->len + 1);
        if (str == NULL) {
            PARSER_ERR(parser, "out of memory");
            return NULL;
        }

		memcpy(&str[len], ident->start, ident->len);
		len += ident->len;
	}

    *end = lastref;

	str[len] = '\0';
    return str;
}

NodeRef node_ident_parse(Parser* parser) {
    NodeIdent* out = malloc(sizeof(NodeIdent));
    RET_IF_OOM(parser,out);
    out->vtable = &NODE_IMPL_IDENT;
    char* str = ident_parse(parser, &out->first_token, &out->last_token);
    out->name = str;

    for (int i = parser->current_scope; i >= 0; i--) {
        SymbolEntry* entry = symbols_get(&parser->scopes[i], out->name);
        if (entry != NULL) {
            out->scope = i;
            goto found_scope;
        }
    }

    RET_ERROR(parser, "identifier not found");

found_scope:
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
