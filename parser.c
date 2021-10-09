#include "tokenizer.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

inline static size_t table_hash(char* str) {
    size_t hash = 5381;
    int c;

    while ((c = *(str++)))
        hash = ((hash << 5) + hash) + (size_t)c; /* hash * 33 + c */

    return hash;
}

#include "parser_type.c"

typedef enum {
	NODE_LET,
	NODE_CONST,
	NODE_FUNC,
	NODE_IF,

	NODE_BLOCK,

	NODE_IDENT,
	NODE_LIT,
	NODE_OP,
	NODE_OP_UNARY,
	NODE_FUNC_CALL,
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
	bool is_mut;
} NodeLet;

typedef struct {
	bool is_mut;
	char* name;
	NodeExpr* type;
} FuncArg;

typedef struct {
	Node node;
	size_t len;
	Node* nodes[0];
} NodeBlock;

typedef struct {
	Node node;
	char* ident;
	NodeExpr* ret_type;

	bool is_varardic;
	char* arg_varardic; // ident

	NodeBlock* body; // optional

	size_t args_len;
	FuncArg args[];
} NodeFunc;

typedef NodeLet NodeConst;

typedef struct {
	NodeExpr expr;
	NodeExpr* lhs;
	NodeExpr* rhs;
} NodeOp;

typedef struct {
	NodeExpr expr;
	NodeExpr* rhs;
	uint64_t data; // for pointer/array types only
} NodeOpUnary;

typedef struct {
	NodeExpr expr;
	size_t scope;
	char* name;
} NodeIdent;

typedef struct {
	NodeExpr expr;
	NodeExpr* func;
	size_t args_len;
	NodeExpr* args[];
} NodeFuncCall;

typedef struct {
	NodeExpr* condition;
	NodeBlock* body;
} NodeIfClause;

typedef struct {
	Node node;
	size_t clauses_len;
	NodeIfClause clauses[];
} NodeIf;

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
	Token tmp = parser->current;

	Token token;
	do {
		token = tok_next(&parser->tok);
		parser->current = token;
	} while (token.type == TOKEN_COMMENT || token.type == TOKEN_COMMENT_MULTI);
	return tmp;
}

static Node* parse_statement(Parser* parser);
static NodeExpr* parse_expr(Parser* parser);
static TypeRef eval_type(Parser* parser, NodeExpr* expr);

#define RET_IF_NULL(expr) do { \
		if ((expr) == NULL) { return NULL; } \
	} while(0)
#define RET_IF_OOM(expr) do { \
		if ((expr) == NULL) { parser->error = "out of memory"; return NULL; } \
	} while(0)

#define RET_ERROR(err) do { \
	parser->error = "out of memory"; \
	return NULL; \
	} while(0)


#define RET_IF_NOT(token, type_, err) do { \
		if ((token).type != type_) { parser->error = err; return NULL; } \
	} while(0)

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
			TypeRef inner = eval_type(parser, unary->rhs);
			RET_IF_NULL(inner);

			TypeRef ref = table_add(&parser->types, TYPE_ANON, (TypeUnderlying){.tag = TYPE_PTR, .data = unary->data, .child=inner});
			RET_IF_OOM(ref);

			return ref;
		}
		if (expr->node.token.type == TOKEN_QUESTION) {
			TypeRef inner = eval_type(parser, unary->rhs);
			RET_IF_NULL(inner);

			TypeUnderlying underlying = inner->type;
			if (underlying.tag != TYPE_SLICE && underlying.tag != TYPE_PTR) UNREACHABLE();
			underlying.data &= TYPE_OPT;
			TypeRef ref = table_add(&parser->types, TYPE_ANON, underlying);
			RET_IF_OOM(ref);

			return ref;
		}
		if (expr->node.token.type == TOKEN_BRACKET_LEFT) {
			TypeRef inner = eval_type(parser, unary->rhs);
			RET_IF_NULL(inner);
			if (unary->data != 0 && unary->data != TYPE_MUT) {
				//array type
				Token* tok = (Token*)unary->data;
				char* len_str = malloc(tok->len+1);
				RET_IF_OOM(len_str);
				memcpy(len_str, tok->start, tok->len);
				len_str[tok->len] = '\0';

				TypeRef ref = table_add(&parser->types, TYPE_ANON, (TypeUnderlying){.tag = TYPE_ARRAY, .data = atoi(len_str), .child = inner});
				RET_IF_OOM(ref);

				free(len_str);

				return ref;
			} else {
				// slice type
				TypeRef ref = table_add(&parser->types, TYPE_ANON, (TypeUnderlying){.tag = TYPE_SLICE, .data = unary->data, .child = inner});
				RET_IF_OOM(ref);
				return ref;
			}
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

static NodeExpr* parse_literal(Parser* parser) {
	Token tok = consume(parser);
	if (tok.type != TOKEN_LIT_INT && tok.type != TOKEN_LIT_FLOAT && tok.type != TOKEN_LIT_STR) {
		RET_ERROR("expected int, float, or string literal");
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

	RET_ERROR("identifier not declared");
}

static NodeExpr* parse_grouping(Parser* parser) {
	if (parser->current.type != TOKEN_PAREN_LEFT) {
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

static NodeExpr* parse_func_call(Parser* parser) {
	NodeExpr* func = parse_grouping(parser);
	RET_IF_NULL(func);
	if (!CHECK(TOKEN_PAREN_LEFT)) {
		return func;
	}
	Token left = consume(parser);

	TypeUnderlying* func_type = &func->type->type;
	if (func_type->tag != TYPE_FUNC) {
		RET_ERROR("lhs of function call is not a function");
	}

	TypeFunc* func_data = (void*)func_type->data;

	NodeFuncCall* call = ALLOC_N(NodeFuncCall, sizeof(NodeExpr*) * 1);
	RET_IF_OOM(call);
	call->expr.node.type = NODE_FUNC_CALL;
	call->expr.node.token = left;
	call->expr.type = func_type->child;
	call->func = func;
	call->args_len = 0;

	if (CHECK(TOKEN_PAREN_RIGHT)) {
		if (func_data->args_len != 0) {
			RET_ERROR("function call has wrong # of args");
		}

		consume(parser);
		return (NodeExpr*)call;
	}

	size_t cap = 1;
	for (;;) {
		NodeExpr* arg = parse_expr(parser);
		RET_IF_NULL(arg);
	
		if (func_data->args_len <= call->args_len) {
			if (!func_data->varardic) {
				RET_ERROR("function call has too many arguments");
			}
		} else {
			if (!type_can_coerce(arg->type, func_data->args[call->args_len])) {
				RET_ERROR("argument has incompatible type");
			}
		}

		if (cap < call->args_len + 1) {
			cap *= 2;
			call = REALLOC_N(NodeFuncCall, call, sizeof(NodeExpr*) * cap);
			RET_IF_OOM(call);
		}

		call->args[call->args_len] = arg;
		call->args_len += 1;
	}

	if (func_data->args_len != call->args_len) RET_ERROR("function call has too few arguments");

	return (NodeExpr*)call;
}

#define IS_OP_UNARY(type) ((type) == TOKEN_ADD || (type) == TOKEN_SUB || (type) == TOKEN_MUL || (type) == TOKEN_BIT_NOT || (type) == TOKEN_BOOL_NOT || (type) == TOKEN_BIT_AND || (type) == TOKEN_QUESTION)
static NodeExpr* parse_unary(Parser* parser) {
	if (!IS_OP_UNARY(parser->current.type) && !CHECK(TOKEN_BRACKET_LEFT)) return parse_func_call(parser);
	Token op = consume(parser);
	uint64_t data = 0;
	if (op.type == TOKEN_BRACKET_LEFT) {
		if (!CHECK(TOKEN_BRACKET_RIGHT)) {
			Token num = consume(parser);
			if (num.type != TOKEN_LIT_INT) {
				RET_ERROR("expected integer literal or ]");
			}

			Token* mem = ALLOC(Token);
			memcpy(mem, &num, sizeof(Token));
			data = (uint64_t)mem;
		}
	}

	if ((op.type == TOKEN_BRACKET_LEFT || op.type == TOKEN_MUL) && data == 0) {
		if (CHECK(TOKEN_MUT)) {
			consume(parser);
			data = TYPE_MUT;
		}
	}

	NodeExpr* rhs = parse_unary(parser);
	RET_IF_NULL(rhs);

	NodeOpUnary* out = ALLOC(NodeOpUnary);
	RET_IF_OOM(out);
	out->expr.node.type = NODE_OP_UNARY;
	out->expr.node.token = op;
	out->data = data;

	switch (rhs->type->type.tag) {
	case TYPE_INT:
	case TYPE_UINT:
	case TYPE_FLOAT:
		if (op.type != TOKEN_ADD && op.type != TOKEN_SUB && op.type != TOKEN_BIT_NOT && op.type != TOKEN_BIT_AND) {
			RET_ERROR("invalid type (not bool) for unary operator !");
		}
		break;
	case TYPE_BOOL:
		if (op.type != TOKEN_BOOL_NOT && op.type != TOKEN_BIT_AND) {
			RET_ERROR("invalid type bool for unary operator");
		}
		break;
	case TYPE_TYPE:
		if (op.type != TOKEN_QUESTION && op.type != TOKEN_BRACKET_LEFT && op.type != TOKEN_MUL) {
			RET_ERROR("invalid type type for unary operator");
		}

		TypeRef inner = eval_type(parser, rhs);
		if (!type_is_runtime(inner)) {
			RET_ERROR("must have runtime inner type");
		}

		if (op.type == TOKEN_QUESTION) {
			if (inner->type.tag != TYPE_SLICE && inner->type.tag != TYPE_PTR) {
				RET_ERROR("optional must be pointer or slice type");
			}
		}

		break;
	default:
		if (op.type != TOKEN_BIT_AND) 
			RET_ERROR("invalid type for unary operator");
		break;
	}
	out->data = 0;

	if (op.type == TOKEN_BIT_AND) {
		// register pointer type
		TypeRef ptr_type = table_add(&parser->types, TYPE_ANON, (TypeUnderlying){.child = rhs->type, .data = 0, .tag = TYPE_PTR});
		RET_IF_OOM(ptr_type);
		out->expr.type = ptr_type;
	} else {
		out->expr.type = rhs->type;
	}
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
			lhs = (NodeExpr*)out;\
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
	// TODO: update to ensure lhs
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
	out->lhs = lhs;
	out->rhs = rhs;
	return out;
}

static NodeExpr* parse_expr(Parser* parser) {
	return parse_assign(parser);
}

static NodeBlock* parse_block_no_scope(Parser* parser) {
	Token brace = consume(parser); // {
	RET_IF_NOT(brace, TOKEN_BRACE_LEFT, "expected '{' to start block");

	NodeBlock* block = ALLOC_N(NodeBlock, 2*sizeof(Node*));
	RET_IF_OOM(block);

	block->node.type = NODE_BLOCK;
	block->node.token = brace;
	block->len = 0;

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

	consume(parser); // consume }
}

static NodeBlock* parse_block(Parser* parser) {
	parser->current_scope++;
	symbols_init(&parser->scopes[parser->current_scope]);

	NodeBlock* block = parse_block_no_scope(parser);

	symbols_free(&parser->scopes[parser->current_scope]);
	parser->current_scope--;

	return block;
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

	if (!type_is_runtime(type)) {
		RET_ERROR("let declarations must have runtime types");
	}

	return symbols_add(table, (SymbolEntry){
		.name = node->ident,
		.decl = &node->node,
		.type = type});
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
		if (type == NULL) {
			RET_ERROR("let statement cannot have implicit type without assignment");
		}

		consume(parser);

		out = ALLOC(NodeLet);
		RET_IF_OOM(out);
		out->node.type = NODE_LET;
		out->node.token = let;
		out->type = type;
		out->ident = ident;

		register_let(parser, out);

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
	out->is_mut = is_mut;
	out->type = type;
	out->ident = ident;
	out->expr = expr;

	register_let(parser, out);

	return out;
}

static NodeConst* parse_const(Parser* parser) {
	Token cnst = consume(parser); // assume const or type keyword

	char* ident = parse_ident(parser);
	RET_IF_NULL(ident);

	NodeExpr* type = NULL;
	if (cnst.type == TOKEN_CONST && !CHECK(TOKEN_EQ)) {
		type = parse_expr(parser);
	}

	Token eq = consume(parser);
	RET_IF_NOT(eq, TOKEN_EQ, "expected '=' symbol");

	NodeExpr* val = parse_expr(parser);
	RET_IF_NULL(val);

	Token semicolon = consume(parser);
	RET_IF_NOT(semicolon, TOKEN_SEMICOLON, "expected end of const statement");

	NodeConst* out =  ALLOC(NodeConst);
	RET_IF_OOM(out);
	out->node.type = NODE_CONST;
	out->node.token = cnst;
	out->is_mut = false;
	out->type = type;
	out->ident = ident;
	out->expr = val;

	TypeRef type_ref = NULL;
	if (type != NULL) {
		type_ref = eval_type(parser, type);
		RET_IF_NULL(type_ref);

		if (!type_can_coerce(val->type, type_ref)) {
			RET_ERROR("incompatible types in const statement");
		}
	} else {
		type_ref = val->type;		
	}

	SymbolEntry entry;
	entry.name = ident;
	entry.type = type_ref;
	entry.value = NULL;
	entry.decl = &out->node;
	if (type_is_eq(entry.type, parser->types.type_type)) {
		entry.value = eval_type(parser, val);
	}
	symbols_add(&parser->scopes[parser->current_scope], entry);

	return out;
}

static NodeFunc* parse_func(Parser* parser) {
	Token func = consume(parser); // assume func keyword

	char* ident =  parse_ident(parser);
	RET_IF_NULL(ident);

	Token lparen = consume(parser);
	RET_IF_NOT(lparen, TOKEN_PAREN_LEFT, "expected '(' in func declaration");

	NodeFunc* out = ALLOC_N(NodeFunc, sizeof(FuncArg) * 1);
	out->node.type = NODE_FUNC;
	out->node.token = func;
	out->ident = ident;

	out->is_varardic = false;
	out->arg_varardic = NULL;

	parser->current_scope++;
	symbols_init(&parser->scopes[parser->current_scope]);

	size_t cap = 1;
	for (;;) {
		// TODO: varardic...
	
		if (out->args_len >= cap) {
			out = REALLOC_N(NodeFunc, out, sizeof(FuncArg) * (cap *= 2));
			RET_IF_NULL(out);
		}
	
		bool mut = false;
		if (CHECK(TOKEN_MUT)) {
			mut = true;
			consume(parser);
		}

		char* name = parse_ident(parser);
		RET_IF_NULL(name);

		NodeExpr* type = parse_expr(parser);
		RET_IF_NULL(type);

		if (!type_is_eq(type->type, parser->types.type_type)) {
			RET_ERROR("argument type must be a type");
		}

		out->args[out->args_len].name = name;
		out->args[out->args_len].type = type;
		out->args[out->args_len].is_mut = mut;
		out->args_len++;

		TypeRef type_val = eval_type(parser, type);
		if (type_val == NULL) {
			RET_ERROR("couldn't parse type in function argument");
		}
		symbols_add(&parser->scopes[parser->current_scope], (SymbolEntry){.decl = (Node*)type, .name = name, .type = type_val, .value = NULL});
		
		if (CHECK(TOKEN_PAREN_RIGHT)) {
			break;
		}
	}

	out->ret_type = parse_expr(parser);
	RET_IF_NULL(out->ret_type);

	out->body = NULL;

	if (CHECK(TOKEN_SEMICOLON))
		return out;

	out->body = parse_block_no_scope(parser);
	RET_IF_NULL(out->body);

	symbols_free(&parser->scopes[parser->current_scope]);
	parser->current_scope--;

	return out;
}

static NodeIf* parse_if(Parser* parser) {
	Token if_ = consume(parser);

	NodeExpr* condition = parse_expr(parser);
	RET_IF_NULL(condition);
	if (!type_is_eq(condition->type, parser->types.type_bool)) {
		RET_ERROR("expected bool in if condition");
	}

	NodeBlock* body = parse_block(parser);
	RET_IF_NULL(body);

	NodeIf* out = ALLOC_N(NodeIf, 1 * sizeof(NodeIfClause));
	RET_IF_OOM(out);
	out->node.token = if_;
	out->node.type = NODE_IF;
	out->clauses[0].condition = condition;
	out->clauses[0].body = body;
	out->clauses_len = 1;

	size_t cap = 1;
	bool has_else = false;
	for (;;) {
		if (!CHECK(TOKEN_ELSE)) break;

		if (out->clauses_len >= cap) {
			cap *= 2;
			out = REALLOC_N(NodeIf, out, cap * sizeof(NodeIfClause));
			RET_IF_OOM(out);
		}


		Token else_ = consume(parser);
		NodeExpr* condition = NULL;

		if (CHECK(TOKEN_IF)) {
			if (has_else) {
				RET_ERROR("can't have 'else if' after 'else'");
			}

			consume(parser);

			condition = parse_expr(parser);
			RET_IF_NULL(condition);
			if (!type_is_eq(condition->type, parser->types.type_bool)) {
				RET_ERROR("expected bool in else if condition");
			}
		} else {
			has_else = true;
		}

		NodeBlock* body = parse_block(parser);
		RET_IF_NULL(body);

		out->clauses[out->clauses_len].condition = condition;
		out->clauses[out->clauses_len].body = body;
		out->clauses_len += 1;
	}

	return out;
}

static Node* parse_statement(Parser* parser) {
	if (CHECK(TOKEN_LET)) {
		return AS_NODE(parse_let(parser));
	} else if (CHECK(TOKEN_BRACE_LEFT)) {
		return AS_NODE(parse_block(parser));
	} else if (CHECK(TOKEN_TYPE) || CHECK(TOKEN_CONST)) {
		return AS_NODE(parse_const(parser));
	} else if (CHECK(TOKEN_FUNC)) {
		return AS_NODE(parse_func(parser));
	} else if (CHECK(TOKEN_IF)) {
		return AS_NODE(parse_if(parser));	
	} else {
		Node* expr = AS_NODE(parse_expr(parser));
		RET_IF_NULL(expr);

		Token semi = consume(parser);
		RET_IF_NOT(semi, TOKEN_SEMICOLON, "expected ';'");
		return expr;
	}
}
