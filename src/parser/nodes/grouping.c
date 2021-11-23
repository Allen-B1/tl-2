#include "grouping.h"
#include "ident.h"
#include "literal.h"
#include "op_binary.h"

#define CHECK(type_) (parser_getpeek(parser)->type == (type_))

NodeRef node_grouping_parse(Parser* parser) {
    if (!CHECK(TOKEN_PAREN_LEFT)) {
        if (CHECK(TOKEN_IDENT)) {
            return node_ident_parse(parser);
        } else {
            return node_literal_parse(parser);
        }
    }

    TokenRef left = parser_consume(parser);
    NodeRef e = node_op_binary_parse(parser);
    RET_IF_ERR(parser, e);
    
    TokenRef right;
    if (!parser_consume_if(parser, TOKEN_PAREN_RIGHT, &right)) 
        RET_ERROR(parser, "expected right paren");

    return e;
}