#include "let.h"
#include "ident.h"
#include "op_binary.h"

TokenRef node_let_token(const Parser* parser, NodeLet* node) {
    return node->kwd;
}

NodeRefSlice node_let_children(const Parser* parser, NodeLet* let)  {
    return (NodeRefSlice){.data = &let->type, .len=2};
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
NodeVTable NODE_IMPL_LET = {
    .name = "Let",
    .token = node_let_token,
    .children = node_let_children
};
#pragma GCC diagnostic pop

// the current token must be the `let`, `static`, or `const` keyword; visiblity modifiers are passed in
// syntax: let [mut] IDENT [TYPE] [= VALUE];
// syntax: let [mut] [IDENT ,*] [TYPE]; (wip)
NodeRef node_let_parse(Parser* parser, TokenRef visibility) {
    Token* kwd_token = parser_getpeek(parser);
    if (!(kwd_token->type == TOKEN_CONST || kwd_token->type == TOKEN_LET || kwd_token->type == TOKEN_STATIC)) {
        RET_ERROR(parser, "expected let, const, or static declaration");
    }
    TokenRef kwd = parser_consume(parser);

    Token* mut_token = parser_getpeek(parser);
    TokenRef mut = TOKREF_ERR;
    if (mut_token->type == TOKEN_MUT) {
        mut = parser_consume(parser);
    }

    // TODO: add multiple declarations
    TokenRef ident_start, ident_end;
    char* name = ident_parse(parser, &ident_start, &ident_end);
    if (name == NULL) {
        RET_ERROR(parser, "could not parse name");
    }

    NodeRef type = NODE_ERR;
    TokenRef eq = TOKREF_ERR;
    NodeRef value = NODE_ERR;
    if (!parser_peek_is(parser, TOKEN_EQ)) {
        // type
        type = node_op_binary_parse(parser);
        RET_IF_ERR(parser, type);
    } 

    if (parser_peek_is(parser, TOKEN_EQ)) {
        eq = parser_consume(parser);
        value = node_op_binary_parse(parser);
        RET_IF_ERR(parser, value);
    }

    if (type == NODE_ERR && value == NODE_ERR) {
        RET_ERROR(parser, "variable declaration must have type or value");
    }

    if (!parser_peek_is(parser, TOKEN_SEMICOLON)) {
        RET_ERROR(parser, "expected ;");
    }
    parser_consume(parser); // semicolon

    NodeLet* node = malloc(sizeof(NodeLet));
    RET_IF_OOM(parser, node);

    node->vtable = &NODE_IMPL_LET;
    node->kwd = kwd;
    node->mut = mut;
    node->linkage = visibility;
    node->ident_start = ident_start;
    node->ident_end = ident_end;
    node->ident_name = name;
    node->type = type;
    node->value = value;
    return parser_addnode(parser, node);
}