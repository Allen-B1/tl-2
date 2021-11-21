#include "symbols.h"

void symbols_init(SymbolTable* table) {
	rhmap_init(&table->symbols, 64, rhmap_djb2_str, rhmap_eq_str);
}

bool symbols_add(SymbolTable* table, char* name, SymbolEntry entry) {
	if (rhmap_has(&table->symbols, name)) {
		return false;
	}

	SymbolEntry* ptr = malloc(sizeof(SymbolEntry));
	if (ptr == NULL) return false;
	memcpy(ptr, &entry, sizeof(SymbolEntry));

	return rhmap_set(&table->symbols, name, ptr);
}

bool symbols_add_builtin(SymbolTable* table, TypeTable* types) {
	#define ENTRY(name_, type_) symbols_add(table, #name_, (SymbolEntry){.node = NODE_ERR, .type = type_ });
	#define TYPE(name_, typeref) symbols_add(table, #name_, (SymbolEntry){ .node = NODE_ERR, .type = TYPEREF_TYPE, .ref_self= typeref});
	TYPE(i8, TYPEREF_I8); TYPE(i16, TYPEREF_I16); TYPE(i32, TYPEREF_I32); TYPE(i64, TYPEREF_I64); TYPE(isize, TYPEREF_ISIZE);
	TYPE(u8, TYPEREF_U8); TYPE(u16, TYPEREF_U16); TYPE(u32, TYPEREF_U32); TYPE(u64, TYPEREF_U64); TYPE(usize, TYPEREF_USIZE);
	TYPE(f16, TYPEREF_F16); TYPE(f32, TYPEREF_F32); TYPE(f64, TYPEREF_F64); TYPE(f64x, TYPEREF_F64X);
	TYPE(type, TYPEREF_TYPE); TYPE(void, TYPEREF_VOID); TYPE(bool, TYPEREF_BOOL);

	ENTRY("true", TYPEREF_BOOL);
	ENTRY("false", TYPEREF_BOOL);
	ENTRY("null", TYPEREF_VOID);
	return true;
}

SymbolEntry* symbols_get(SymbolTable* table, const char* name) {
	return rhmap_get(&table->symbols, (void*)name);
}

void symbols_free(SymbolTable* table) {
	// memory leak!
	return rhmap_deinit(&table->symbols);
}
