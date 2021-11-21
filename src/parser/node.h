#ifndef _NODE_H
#define _NODE_H

#include "../utils.h"
#include <stdint.h>
#include "parser_forward.h"
#include "types.h"

#define NODE_ERR SIZE_MAX

typedef struct Node Node;

typedef struct NodeVTable {
	const char* name;
	TypeRef (*type)( const Parser*, Node*);
	NodeRefSlice (*children)( const Parser*, Node*);
	TokenRef (*token)( const Parser*, Node*);
} NodeVTable;

struct Node {
	NodeVTable* vtable;
};

#endif