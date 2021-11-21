#ifndef _SYMBOLS_H
#define _SYMBOLS_H

#include <rhmap.h>
#include "../tokenizer/tokenizer.h"
#include "parser_forward.h"
#include "types.h"
#include "node.h"

typedef struct {
	NodeRef node;
	TypeRef type;
    TypeRef ref_self; // if type is TYPE_TYPE
} SymbolEntry;

typedef struct {
    RhMap /* SymbolEntry* */ symbols;
} SymbolTable;

void symbols_init(SymbolTable* table);
bool symbols_add(SymbolTable* table, char* name, SymbolEntry entry);
bool symbols_add_builtin(SymbolTable* table, TypeTable* types);
SymbolEntry* symbols_get(SymbolTable* table, const char* name);
void symbols_free(SymbolTable* table);

#endif