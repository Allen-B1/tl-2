typedef struct {
	const char* name; // unowned
	Node* decl;
	TypeRef type;
	TypeRef value;
	size_t hash;
} SymbolEntry;

typedef struct {
	size_t len;
	// must be power of 2
	size_t cap;
	SymbolEntry* buckets;
} SymbolTable;

void symbols_init(SymbolTable* table) {
	table->cap = 0;
	table->len = 0;
	table->buckets = NULL;
	return;
}

bool symbols_grow(SymbolTable* table, size_t cap) {
	SymbolEntry* new_buckets = malloc(sizeof(SymbolEntry) * cap);

	if (new_buckets == NULL) return false;
	for (size_t i = 0; i < table->cap; i++) {
		if (table->buckets[i].name != NULL) {
			size_t hash = table->buckets[i].hash;
			memcpy(&new_buckets[hash & (cap-1)], &table->buckets[i], sizeof(SymbolEntry));
		}
	}
	table->buckets = new_buckets;
	table->cap = cap;

	return true;
}

bool symbols_add(SymbolTable* table, SymbolEntry entry) {
	if (table->len * 2 >= table->cap) {
		if (!symbols_grow(table, table->cap == 0 ? 16 : table->cap * 2)) return false;		
	}

	size_t hash = table_hash(entry.name);
	entry.hash = hash;

	size_t idx = hash & (table->cap-1);
	for (;;) {
		if (table->buckets[idx].name == NULL) {
			memcpy(&table->buckets[idx], &entry, sizeof(SymbolEntry));
			break;			
		}
		idx = (idx+1) & (table->cap - 1);
	}

	table->len += 1;
	return true;
}

bool symbols_add_global(SymbolTable* table, TypeTable* types) {
	#define ENTRY(name_, type_) symbols_add(table, (SymbolEntry){.name = name_, .value = NULL, .type = type_});
	#define TYPE(name_) symbols_add(table, (SymbolEntry){ .name = #name_, .value = types->type_##name_, .type = types->type_type});
	TYPE(i8); TYPE(i16); TYPE(i32); TYPE(i64);
	TYPE(u8); TYPE(u16); TYPE(u32); TYPE(u64);
	TYPE(f16); TYPE(f32); TYPE(f64); TYPE(f80); TYPE(f128);
	TYPE(type); TYPE(void); TYPE(bool);

	ENTRY("true", types->type_bool);
	ENTRY("false", types->type_bool);
	ENTRY("null", types->type_void);
}

// the returned entry should not be kept in any data structures.
// invalidated after symbols_grow
SymbolEntry* symbols_get(SymbolTable* table, const char* name) {
	size_t hash = table_hash(name);
	size_t start_idx = hash & (table->cap-1);
	size_t idx = start_idx;
	for (;;) {
		if (table->buckets[idx].hash == hash && strcmp(table->buckets[idx].name, name) == 0) {
			return &table->buckets[idx];
		}
		idx = (idx+1) & (table->cap - 1);
		if (idx == start_idx) break;
	}
	return NULL;
}

void symbols_free(SymbolTable* table) {
	free(table->buckets);
	table->len = 0;
	table->cap = 0;
}
