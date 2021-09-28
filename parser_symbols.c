typedef struct {
	const char* name;
	Node* decl;
	TypeRef type;
	TypeRef value;
} SymbolEntry;

typedef struct {
	size_t len;
	size_t cap;
	SymbolEntry* buckets;
} SymbolTable;

void symbols_init(SymbolTable* table) {
	table->cap = 0;
	table->len = 0;
	table->buckets = NULL;
	return;
}

void symbols_resize(SymbolTable* table) {

}

void symbols_add(SymbolTable* table, SymbolEntry entry) {
	memcpy(&entry
}
