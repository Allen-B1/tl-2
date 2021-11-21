#include "types.h"

bool type_is_eq(TypeTable* table, TypeRef from, TypeRef to) {
	#define HEADER \
		TypeEntry* from_entry = typetable_get(table, (from)); \
		TypeEntry* to_entry = typetable_get(table, (to))

	#define FROM (from_entry->type)
	#define TO (to_entry->type)

	HEADER;

	if (FROM.tag != TO.tag) return false;
	switch (FROM.tag) {
	case TYPE_ARRAY:
	case TYPE_PTR:
	case TYPE_SLICE:
		return type_is_eq(table, FROM.child, TO.child) && FROM.data == TO.data;

	case TYPE_UINT:
	case TYPE_INT:
	case TYPE_FLOAT:
		return FROM.data == TO.data;

	case TYPE_BOOL: return FROM.data == TO.data;
	case TYPE_VOID:
	case TYPE_TYPE:
		return true;

	case TYPE_ENUM: return from == to;
	case TYPE_UNION:
	case TYPE_STRUCT:;
		TypeFields* from_fields = (TypeFields*)FROM.data;
		TypeFields* to_fields = (TypeFields*)TO.data;
		if (from_fields->fields.len != to_fields->fields.len) return false;
		for (size_t i = 0; i < to_fields->fields.len; i++) {
			if (!type_is_eq(table, 
					((TypeFieldsField*)arrlist_get(&to_fields->fields, i))->type,
					((TypeFieldsField*)arrlist_get(&from_fields->fields, i))->type)) return false;
		}
		return true;

	case TYPE_FUNC:;
		TypeFuncData* from_data = UINT_TO_PTR(FROM.data);
		TypeFuncData* to_data = UINT_TO_PTR(TO.data);

		if (from_data->arg_types.len != to_data->arg_types.len || from_data->varardic != to_data->varardic) return false;
		if (!type_is_eq(table, from_data->ret_type, to_data->ret_type)) return false;

		for (size_t i = 0; i < from_data->arg_types.len; i++) {
			if (!type_is_eq(table, 
				(TypeRef)arrlist_get(&to_data->arg_types, i),
				(TypeRef)arrlist_get(&from_data->arg_types, i))) return false;
		}
		return true;
	}
}

bool type_can_coerce(TypeTable* table, TypeRef from, TypeRef to) {
	HEADER;

	switch (TO.tag) {
	case TYPE_VOID: return FROM.tag == TYPE_VOID;
	case TYPE_BOOL: return (FROM.tag == TYPE_VOID && (TO.data & TYPE_OPT) != 0) || (FROM.tag == TYPE_BOOL && ((TO.data & TYPE_OPT) != 0 || (FROM.data & TYPE_OPT) == 0));
	case TYPE_TYPE: return FROM.tag == TYPE_TYPE;

	case TYPE_UINT:
		if (FROM.tag == TYPE_INT && FROM.data == 0) return true;
		return FROM.tag == TYPE_UINT && FROM.data == TO.data;
	case TYPE_INT:
		return FROM.tag == TYPE_INT && (FROM.data == 0 || FROM.data == TO.data);
	case TYPE_FLOAT:
		return FROM.tag == TYPE_FLOAT && (FROM.data == 0 || FROM.data == TO.data);

	case TYPE_ENUM:
	case TYPE_UNION:
	case TYPE_STRUCT:

	case TYPE_ARRAY:
		return type_is_eq(table, from, to);
	case TYPE_PTR:
	case TYPE_SLICE:
		return (FROM.tag == TYPE_VOID && (TO.data & TYPE_OPT) != 0) ||
			((FROM.tag == TYPE_PTR || FROM.tag == TO.tag) && type_is_eq(table, FROM.child, TO.child) && 
				((TO.data & TYPE_OPT) != 0 || (FROM.data & TYPE_OPT) == 0) &&
				((TO.data & TYPE_MUT) == 0 || (FROM.data & TYPE_MUT) != 0));

	case TYPE_FUNC:
		return type_is_eq(table, from, to);
	}
}

bool type_can_cast(TypeTable* table, TypeRef from, TypeRef to) {
	HEADER;

	switch (TO.tag) {
	case TYPE_VOID: return FROM.tag == TYPE_VOID;
	case TYPE_BOOL: return (FROM.tag == TYPE_VOID && (TO.data & TYPE_OPT) != 0) || FROM.tag == TYPE_BOOL;
	case TYPE_TYPE: return FROM.tag == TYPE_TYPE;

	case TYPE_FLOAT:
	case TYPE_UINT:
	case TYPE_INT: return FROM.tag == TYPE_INT || FROM.tag == TYPE_BOOL || FROM.tag == TYPE_UINT || FROM.tag == TYPE_ENUM || FROM.tag == TYPE_FLOAT;

	case TYPE_ENUM: return FROM.tag == TYPE_INT || FROM.tag == TYPE_BOOL || FROM.tag == TYPE_UINT || FROM.tag == TYPE_ENUM;
	case TYPE_UNION:
	case TYPE_STRUCT:

	case TYPE_ARRAY:
		return type_is_eq(table, from, to);
	case TYPE_SLICE:
		return (FROM.tag == TYPE_VOID && (TO.data & TYPE_OPT) != 0) ||
			((FROM.tag == TYPE_PTR || FROM.tag == TYPE_SLICE) && type_is_eq(table, FROM.child, TO.child));
	case TYPE_PTR:
		return (FROM.tag == TYPE_VOID && (TO.data & TYPE_OPT) != 0) ||
			(FROM.tag == TYPE_PTR);

	case TYPE_FUNC:
		return type_is_eq(table, from, to);
	}

	#undef HEADER
	#undef FROM
	#undef TO
}

bool type_is_runtime(TypeTable* table, TypeRef type) {
	TypeEntry* type_entry = typetable_get(table, type);

	if (type_entry->type.tag == TYPE_TYPE) return false;
	if (type_entry->type.tag == TYPE_INT && type_entry->type.data == 0) return false;
	return true;
}

TypeRef typetable_add(TypeTable* table, const char* name, Type type) {
	size_t ref = table->entries.len;

	TypeEntry* entry = malloc(sizeof(TypeEntry));
	if (entry == NULL) {
		fprintf(stderr, "typetable_add: OOM\n");
		abort();
	}
	entry->name = name;
	entry->type = type;

	if (!arrlist_add(&table->entries, entry)) {
		fprintf(stderr, "typetable_add: OOM\n");
		abort();
	}

	return ref;
}

TypeEntry* typetable_get(TypeTable* table, TypeRef ref) {
	return arrlist_get(&table->entries, ref);
}

void typetable_init(TypeTable* table) {
	arrlist_init(&table->entries, 64);

	#define MAKE(ident, name_, info) do { \
		TypeEntry* entry = malloc(sizeof(TypeEntry)); \
		if (entry == NULL) abort(); \
		entry->name = name_; \
		entry->type = (Type)info; \
		table->entries.data[TYPEREF_##ident] = entry; \
	} while(0)
	MAKE(VOID, "void", ((Type){.tag=TYPE_VOID, .data=0}));
	MAKE(BOOL, "bool", ((Type){.tag=TYPE_BOOL, .data=0}));

	MAKE(I8, "i8", ((Type){.tag=TYPE_INT, .data=8}));
	MAKE(I16, "i16", ((Type){.tag=TYPE_INT, .data=16}));
	MAKE(I32, "i32", ((Type){.tag=TYPE_INT, .data=32}));
	MAKE(I64, "i64", ((Type){.tag=TYPE_INT, .data=64}));
	MAKE(ISIZE, "isize", ((Type){.tag=TYPE_INT, .data=1}));

	MAKE(U8, "u8", ((Type){.tag=TYPE_UINT, .data=8}));
	MAKE(U16, "u16", ((Type){.tag=TYPE_UINT, .data=16}));
	MAKE(U32, "u32", ((Type){.tag=TYPE_UINT, .data=32}));
	MAKE(U64, "u64", ((Type){.tag=TYPE_UINT, .data=64}));
	MAKE(USIZE, "usize", ((Type){.tag=TYPE_UINT, .data=1}));

	MAKE(F16, "f16", ((Type){.tag=TYPE_FLOAT, .data=16}));
	MAKE(F32, "f32", ((Type){.tag=TYPE_FLOAT, .data=32}));
	MAKE(F64, "f64", ((Type){.tag=TYPE_FLOAT, .data=64}));
	MAKE(F64X, "f64x", ((Type){.tag=TYPE_FLOAT, .data=80}));

	MAKE(GENERIC_INT, "'gint", ((Type){.tag=TYPE_INT, .data=0}));
	MAKE(GENERIC_FLOAT, "'gfloat", ((Type){.tag=TYPE_FLOAT, .data=0}));

	MAKE(TYPE, "type", ((Type){.tag=TYPE_TYPE, .data=0}));
	MAKE(STR, "'str", ((Type){.tag=TYPE_SLICE, .data=0, .child=TYPEREF_U8}));

	table->entries.len = 20;

	#undef MAKE
}

//const char* const TYPE_ANON = "";
