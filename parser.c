#include "tokenizer.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

inline static size_t table_hash(char *str)
{
    size_t hash = 5381;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + (size_t)c; /* hash * 33 + c */

    return hash;
}

#include "parser_type.c"

typedef enum {
	NODE_LET,

	NODE_BLOCK,

	NODE_IDENT,
	NODE_LIT,
	NODE_OP,
	NODE_OP_UNARY,
} NodeType;

typedef struct Node {
	NodeType type;
	Token token;
} Node;

#include "parser_symbols.c"

typedef struct {
	Tokenizer tok;
	Token current;
	const char* error;
	TypeTable types;
	// maximum 256 depth
	SymbolTable scopes[256];
	size_t current_scope;
} Parser;


typedef struct {
	Node node;
	TypeRef type;
} NodeExpr;

typedef struct {
	Node node;
	char* ident;
	NodeExpr* type;
	NodeExpr* expr;
} NodeLet;

typedef struct {
	Node node;
	size_t len;
	Node* nodes[0];
} NodeBlock;


typedef struct {
	NodeExpr expr;
	Node* lhs;
	Node* rhs;
} NodeOp;

typedef struct {
	NodeExpr expr;
	Node* rhs;
	uint32_t data; // for pointer types only
} NodeOpUnary;

typedef struct {
	NodeExpr expr;
	size_t scope;
	char* name;
} NodeIdent;

#define CHECK(typ) (parser->current.type == typ)
#define AS_NODE(expr) ((Node*)(expr))

#define ALLOC(type) (type*)malloc(sizeof(type))
#define ALLOC_N(type, n) (type*)malloc(sizeof(type)+(n))
#define REALLOC_N(type, old, n) (type*)realloc(old, sizeof(type)+(n))

#define UNREACHABLE() do { fprintf(stderr, "unreachable code on line %d\n", __LINE__); exit(2); } while(0) 

void parser_init(Parser* parser, Tokenizer tok) {
	memset(parser, 0, sizeof(Parser));
	parser->tok = tok;
	table_init(&parser->types);
	symbols_init(&parser->scopes[0]);
	symbols_add_global(&parser->scopes[0], &parser->types);
}

// returns current. moves current to next token.
static inline Token consume(Parser* parser) {
	Token token = tok_next(&parser->tok);
	Token tmp = parser->current;
	parser->current = token;
	return tmp;
}

static NodeExpr* parse_expr(Parser* parser);
static NodeLet* parse_let(Parser* parser);
static NodeBlock* parse_block(Parser* parser);
static TypeRef eval_type(Parser* parser, NodeExpr* expr);

#define RET_IF_NULL(expr) do { \
		if ((expr) == NULL) { return NULL; } \
	} while(0);
#define RET_IF_OOM(expr) do { \
		if ((expr) == NULL) { parser->error = "out of memory"; return NULL; } \
	} while(0);

#define RET_IF_NOT(token, type_, err) do { \
		if ((token).type != type_) { parser->error = err; return NULL; } \
	} while(0);

static Node* parse_statement(Parser* parser) {
	if (CHECK(TOKEN_LET)) {
		return AS_NODE(parse_let(parser));
	} else if (CHECK(TOKEN_BRACE_LEFT)) {
		return AS_NODE(parse_block(parser));
	} /* else if (CHECK(TOKEN_CONST)) {
		return AS_NODE(parse_const(parser));
	} else if (CHECK(TOKEN_FUNC)) {
		return AS_NODE(parse_func(parser));
	} */ else {
		Node* expr = parse_expr(parser);
		RET_IF_NULL(expr);

		Token semi = consume(parser);
		RET_IF_NOT(semi, TOKEN_SEMICOLON, "expected ';'");
		return expr;
	}
}

static NodeBlock* parse_block(Parser* parser) {
	Token brace = consume(parser); // {

	NodeBlock* block = ALLOC_N(NodeBlock, 2*sizeof(Node*));
	RET_IF_OOM(block);

	block->node.type = NODE_BLOCK;
	block->node.token = brace;
	block->len = 0;

	parser->current_scope++;
	symbols_init(&parser->scopes[parser->current_scope]);

	size_t cap = 2;
	while (!CHECK(TOKEN_BRACE_RIGHT)) {
		block->len += 1;
		if (block->len > cap) {
			cap *= 2;
			block = REALLOC_N(NodeBlock, block, cap*sizeof(Node*));
			RET_IF_OOM(block);
		}

		Node* stmt = parse_statement(parser);
		block->nodes[block->len-1] = stmt;
	}

	symbols_free(&parser->scopes[parser->current_scope]);
	parser->current_scope--;

	consume(parser); // consume }

	return block;
}

static char* parse_ident(Parser* parser) {
	Token ident = consume(parser);
	RET_IF_NOT(ident, TOKEN_IDENT, "expected ident in symbol");

	char* str = malloc(ident.len+1);
	RET_IF_OOM(str);

	size_t len = 0;
	memcpy(&str[len], ident.start, ident.len);
	len += ident.len;
	for (;;) {
		if (CHECK(TOKEN_COLONS)) {
			consume(parser);

			str = realloc(str, len+2);			
			RET_IF_OOM(str);
	
			memcpy(&str[len], "::", 2);
			len += 2;
		} else { break; }

		Token ident = consume(parser);
		RET_IF_NOT(ident, TOKEN_IDENT, "expected ident in symbol");

		str = realloc(str, len + ident.len + 1);
		RET_IF_OOM(str);

		memcpy(&str[len], ident.start, ident.len);
		len += ident.len;
	}

	str[len] = '\0';

	return str;
}

static bool register_let(Parser* parser, NodeLet* node) {
	SymbolTable* table = &parser->scopes[parser->current_scope];
	
	TypeRef type = NULL;
	if (node->type != NULL) {
		type = eval_type(parser, node->type);
		RET_IF_NULL(type);
	} else {
		type = node->expr->type;
	}

	return symbols_add(table, (SymbolEntry){
		.name = node->ident,
		.decl = node,
		.type = type});
}

static TypeRef eval_type(Parser* parser, NodeExpr* expr) {
	if (!type_is_eq(expr->type, parser->types.type_type)) {
		parser->error = "expression is not of type type";
		return NULL;
	}

	switch (expr->node.type) {
	case NODE_IDENT:;
		NodeIdent* ident = (NodeIdent*)expr;
		SymbolEntry* entry = symbols_get(&parser->scopes[ident->scope], ident->name);
		if (entry == NULL) UNREACHABLE();
		if (entry->value == NULL) UNREACHABLE();
		return entry->value;
	case NODE_OP_UNARY:;
		NodeOpUnary* unary = (NodeOpUnary*)expr;
		if (expr->node.token.type == TOKEN_MUL) { // ptr type
			TypeRef inner = eval_type(parser, expr);
			RET_IF_NULL(inner);

			TypeRef ref = table_add(&parser->types, TYPE_ANON, (TypeUnderlying){.tag = TYPE_PTR, .data = unary->data, .child=inner});
			RET_IF_OOM(ref);

			return ref;
		}

		parser->error = "invalid unary operation";
		return NULL;
	// slices/arrays
	// structs, unions, enums
	default:
		parser->error = "unknown node type when parsing type";
		return NULL;
	}
}

static NodeLet* parse_let(Parser* parser) {
	Token let = consume(parser); // discord let token	

	bool is_mut = false;
	if (CHECK(TOKEN_MUT)) {
		consume(parser);
		is_mut = true;
	}

	char* ident = parse_ident(parser);
	RET_IF_NULL(ident);

	NodeExpr* type = NULL;
	if (!CHECK(TOKEN_EQ)) {
		type = parse_expr(parser);
		RET_IF_NULL(type);

		if (!type_is_eq(type->type, parser->types.type_type)) {
			parser->error = "expression must be a type";
			return NULL;
		}
	}
	
	NodeLet* out = NULL;
	if (CHECK(TOKEN_SEMICOLON)) {
		consume(parser);

		out = ALLOC(NodeLet);
		RET_IF_OOM(out);
		out->node.type = NODE_LET;
		out->node.token = let;
		out->type = type;
		out->ident = ident;
		return out;
	}

	Token equal = consume(parser);
	RET_IF_NOT(equal, TOKEN_EQ, "expected '=' operator");

	NodeExpr* expr = parse_expr(parser);
	RET_IF_NULL(expr);

	Token semicolon = consume(parser);
	RET_IF_NOT(semicolon, TOKEN_SEMICOLON, "expected end of let statement");

	out = ALLOC(NodeLet);
	RET_IF_OOM(out);
	out->node.type = NODE_LET;
	out->node.token = let;
	out->type = type;
	out->ident = ident;
	out->expr = expr;

	return out;
}

static NodeExpr* parse_literal(Parser* parser) {
	Token tok = consume(parser);
	if (tok.type != TOKEN_LIT_INT && tok.type != TOKEN_LIT_FLOAT && tok.type != TOKEN_LIT_STR) {
		parser->error = "expected int, float, or string literal";
		return NULL;
	}

	NodeExpr* out = ALLOC(NodeExpr);
	RET_IF_OOM(out);
	out->node.type = NODE_LIT;
	out->node.token = tok;
	switch (tok.type) {
	case TOKEN_LIT_INT: out->type = parser->types.type_i0;
	case TOKEN_LIT_FLOAT: out->type = parser->types.type_f0;
	case TOKEN_LIT_STR: out->type = parser->types.type_str;
	}
	return out;
}

static NodeIdent* parse_ident_expr(Parser* parser) {
	char* ident = parse_ident(parser);
	RET_IF_NULL(ident);

	NodeIdent* out = ALLOC(NodeIdent);
	RET_IF_OOM(out);

	out->expr.node.type = NODE_IDENT;
	// no associated token

	for (size_t scope = parser->current_scope; scope >= 0; scope--) {
		SymbolTable* table = &parser->scopes[scope];
		SymbolEntry* entry = symbols_get(table, ident);
		if (entry == NULL) {
			continue;
		}
		out->expr.type = entry->type;
		out->scope = scope;
		out->name = ident;
		return out;
	}

	parser->error = "identifier not declared";
	return NULL;
}

static NodeExpr* parse_grouping(Parser* parser) {
	if (parser->current.type != TOKEN_PAREN_RIGHT) {
		if (CHECK(NODE_IDENT)) return parse_ident_expr(parser);
		else return parse_literal(parser);
	}
	Token lhs = consume(parser);

	NodeExpr* expr = parse_expr(parser);
	RET_IF_NULL(expr);

	Token rhs = consume(parser);
	RET_IF_NOT(rhs, TOKEN_PAREN_RIGHT, "expected ')'");

	return expr;
}

#define IS_OP_UNARY(type) ((type) == TOKEN_ADD || (type) == TOKEN_SUB || (type) == TOKEN_MUL || (type) == TOKEN_BIT_NOT || (type) == TOKEN_BOOL_NOT || (type) == TOKEN_BIT_AND)
static Node* parse_unary(Parser* parser) {
	if (!IS_OP_UNARY(parser->current.type)) return parse_grouping(parser);
	Token op = consume(parser);

	NodeExpr* rhs = parse_grouping(parser);
	RET_IF_NULL(rhs);

	NodeOpUnary* out = ALLOC(NodeOpUnary);
	RET_IF_OOM(out);
	out->expr.node.type = NODE_OP_UNARY;
	out->expr.node.token = op;

	switch (rhs->type->type.tag) {
	case TYPE_INT:
	case TYPE_UINT:
	case TYPE_FLOAT:
		if (parser->current.type == TOKEN_BOOL_NOT) {
			parser->error = "invalid type (not bool) for unary operator !";
			return NULL;
		}
		break;
	case TYPE_BOOL:
		if (parser->current.type != TOKEN_BOOL_NOT) {
			parser->error = "invalid type bool for unary operator";
			return NULL;
		}
		break;
	default:
		parser->error = "invalid type for unary operator";
		return NULL;
	}
	out->data = 0;
	out->expr.type = rhs->type;
	out->rhs = rhs;
	return out;
}

#define IS_OP_5(type) ((type) == TOKEN_MUL || (type) == TOKEN_DIV || (type) == TOKEN_MOD || (type) == TOKEN_SHIFT_LEFT || (type) == TOKEN_SHIFT_RIGHT || (type) == TOKEN_BIT_AND)
#define IS_OP_4(type) ((type) == TOKEN_ADD || (type) == TOKEN_SUB || (type) == TOKEN_BIT_OR || (type) == TOKEN_BIT_XOR)
#define IS_OP_3(type) ((type) == TOKEN_CMP_EQ || (type) == TOKEN_CMP_NE || (type) == TOKEN_CMP_LE || (type) == TOKEN_CMP_GE || (type) == TOKEN_CMP_LT || (type) == TOKEN_CMP_GT)
#define IS_OP_2(type) ((type) == TOKEN_BOOL_AND)
#define IS_OP_1(type) ((type) == TOKEN_BOOL_OR)

#define DEFINE_OP_LEFT(name, parse_stronger, test_op, rt_func, err_str) \
	static NodeExpr* parse_##name(Parser* parser) { \
		NodeExpr* lhs = parse_stronger(parser); \
		RET_IF_NULL(lhs); \
\
		for (;;) { \
			if (!test_op(parser->current.type)) return lhs; \
			Token op = consume(parser); \
\
			NodeExpr* rhs = parse_stronger(parser); \
			RET_IF_NULL(rhs); \
\
			NodeOp* out = ALLOC(NodeOp); \
			out->expr.node.type = NODE_OP; \
			out->expr.node.token = op; \
			TypeRef ret_type = rt_func(parser, lhs->type, rhs->type);\
			if (ret_type == NULL) {\
				parser->error = err_str;\
				return NULL;\
			}\
			out->expr.type = ret_type; \
			out->lhs = lhs; \
			out->rhs = rhs; \
\
			lhs = out;\
		}\
	}

inline static TypeRef rt_op(Parser* parser, TypeRef lhs, TypeRef rhs) {
	if (!type_can_coerce(rhs, lhs)) {
		return NULL;
	}
	return lhs;
}

inline static  TypeRef rt_cmp(Parser* parser, TypeRef lhs, TypeRef rhs) {
	if (!type_can_coerce(rhs, lhs)) {
		return NULL;
	}
	return parser->types.type_bool;
}

inline static  TypeRef rt_bool(Parser* parser, TypeRef lhs, TypeRef rhs) {
	if (lhs->type.tag != TYPE_BOOL || rhs->type.tag != TYPE_BOOL) return NULL;
	return parser->types.type_bool;
}

DEFINE_OP_LEFT(op5, parse_unary, IS_OP_5, rt_op, "cannot coerce rhs to lhs in *-class op")
DEFINE_OP_LEFT(op4, parse_op5, IS_OP_4, rt_op,  "cannot coerce rhs to lhs in +-class op")
DEFINE_OP_LEFT(op3, parse_op4, IS_OP_3, rt_cmp,  "cannot coerce rhs to lhs in ==-class op")
DEFINE_OP_LEFT(op2, parse_op3, IS_OP_2, rt_bool,  "cannot coerce rhs or lhs to bool in && op")
DEFINE_OP_LEFT(op1, parse_op2, IS_OP_1, rt_bool, "cannot coerce rhs or lhs to bool in || op")

#define IS_OP_EQ(type) ((type) == TOKEN_EQ || (type) == TOKEN_EQ_ADD || (type) == TOKEN_EQ_SUB || (type) == TOKEN_EQ_MUL || (type) == TOKEN_EQ_DIV || (type) == TOKEN_EQ_MOD || \
	(type) == TOKEN_EQ_BIT_AND || (type) == TOKEN_EQ_BIT_OR || (type) == TOKEN_EQ_BIT_XOR || (type) == TOKEN_EQ_SHIFT_LEFT || (type) == TOKEN_EQ_SHIFT_RIGHT)
static NodeExpr* parse_assign(Parser* parser) {
	// TODO: update to ensure RHS
	NodeExpr* lhs = parse_unary(parser);
	RET_IF_NULL(lhs);

	if (!IS_OP_EQ(parser->current.type)) return lhs;
	Token eq = consume(parser);

	NodeExpr* rhs = parse_assign(parser);
	RET_IF_NULL(rhs);

	if (!type_can_coerce(rhs->type, lhs->type)) {
		parser->error = "cannot coerce rhs to lhs in assignment";
		return NULL;
	}

	NodeOp* out = ALLOC(NodeOp);
	out->expr.node.type = NODE_OP;
	out->expr.node.token = eq;
	out->expr.type = lhs->type;
	out->lhs = AS_NODE(lhs);
	out->rhs = AS_NODE(rhs);
	return out;
}

static NodeExpr* parse_expr(Parser* parser) {
	return parse_assign(parser);
}
