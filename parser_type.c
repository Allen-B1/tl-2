typedef enum {
	TYPE_VOID,
	TYPE_BOOL, // .data = optional
	TYPE_TYPE,

	TYPE_UINT, // .data = bitsize, {.data=1} = usize
	TYPE_INT, // .data = bitsize
	TYPE_FLOAT, // .data = bitsize

	TYPE_ARRAY, // .data = len, .child
	TYPE_PTR, // .data = MUT | OPT, .child
	TYPE_SLICE, // .data = MUT | OPT, .child

	TYPE_STRUCT, // .data = pointer TypeFields*
	TYPE_UNION, // .data = pointer TypeFields*
	TYPE_ENUM, // .child
} TypeTag;

enum {
	TYPE_MUT = 0x2,
	TYPE_OPT = 0x1
};

#define PTR_OF(type, u64) (type*)((u64) & 0x0000ffffffffffff)
#define DATA_OF(type, u64) (type)((u64) & 0xffff000000000000)

typedef struct TypeEntry* TypeRef;
const char* const TYPE_ANON = "";

typedef struct {
	TypeTag tag;
	uint64_t data;
	TypeRef child;
} TypeUnderlying;

typedef struct TypeEntry {
	char* name; // NULL = not present
	TypeUnderlying type;
} TypeEntry;

typedef struct {
	const char* name;
	TypeRef type;
} TypeField;

typedef struct {
	size_t len;
	TypeField data[0];
} TypeFields;

static bool type_is_eq(TypeRef from, TypeRef to) {
	#define FROM (from->type)
	#define TO (to->type)
	if (FROM.tag != TO.tag) return false;
	switch (FROM.tag) {
	case TYPE_ARRAY:
	case TYPE_PTR:
	case TYPE_SLICE:
		return type_is_eq(FROM.child, TO.child) && FROM.data == TO.data;

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
		if (from_fields->len != to_fields->len) return false;
		for (size_t i = 0; i < to_fields->len; i++) {
			if (!type_is_eq(to_fields->data[i].type, from_fields->data[i].type)) return false;
		}
		return true;
	}
}

// can coerce without a cast
static bool type_can_coerce(TypeRef from, TypeRef to) {
	switch (TO.tag) {
	case TYPE_VOID: return FROM.tag == TYPE_VOID;
	case TYPE_BOOL: return (FROM.tag == TYPE_VOID && TO.data & TYPE_OPT != 0) || (FROM.tag == TYPE_BOOL && (TO.data & TYPE_OPT != 0 || FROM.data & TYPE_OPT == 0));
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
		return type_is_eq(from, to);
	case TYPE_PTR:
	case TYPE_SLICE:
		return (FROM.tag == TYPE_VOID && TO.data & TYPE_OPT != 0) ||
			((FROM.tag == TYPE_PTR || FROM.tag == TO.tag) && type_is_eq(FROM.child, TO.child) && 
				(TO.data & TYPE_OPT != 0 || FROM.data & TYPE_OPT == 0) &&
				(TO.data & TYPE_MUT == 0 || FROM.data & TYPE_MUT != 0));
	}
}

static bool type_can_cast(TypeRef from, TypeRef to) {
	switch (TO.tag) {
	case TYPE_VOID: return FROM.tag == TYPE_VOID;
	case TYPE_BOOL: return (FROM.tag == TYPE_VOID && TO.data & TYPE_OPT != 0) || FROM.tag == TYPE_BOOL;
	case TYPE_TYPE: return FROM.tag == TYPE_TYPE;

	case TYPE_FLOAT:
	case TYPE_UINT:
	case TYPE_INT: return FROM.tag == TYPE_INT || FROM.tag == TYPE_BOOL || FROM.tag == TYPE_UINT || FROM.tag == TYPE_ENUM || FROM.tag == TYPE_FLOAT;

	case TYPE_ENUM: return FROM.tag == TYPE_INT || FROM.tag == TYPE_BOOL || FROM.tag == TYPE_UINT || FROM.tag == TYPE_ENUM;
	case TYPE_UNION:
	case TYPE_STRUCT:

	case TYPE_ARRAY:
		return type_is_eq(from, to);
	case TYPE_SLICE:
		return (FROM.tag == TYPE_VOID && TO.data & TYPE_OPT != 0) ||
			((FROM.tag == TYPE_PTR || FROM.tag == TYPE_SLICE) && type_is_eq(FROM.child, TO.child));
	case TYPE_PTR:
		return (FROM.tag == TYPE_VOID && TO.data & TYPE_OPT != 0) ||
			(FROM.tag == TYPE_PTR);
	}

	#undef FROM
	#undef TO
}

#define TYPETABLE_CAP 0x10000

typedef struct {
	size_t count;
	TypeEntry entries[TYPETABLE_CAP];

	#define DECL(name) TypeRef type_##name
	DECL(void); DECL(bool);	
	DECL(i0); DECL(i8); DECL(i16); DECL(i32); DECL(i64);
		DECL(u8); DECL(u16); DECL(u32); DECL(u64); DECL(usize);
	DECL(f0); DECL(f16); DECL(f32); DECL(f64); DECL(f80); DECL(f128);
	DECL(type); DECL(str);
	#undef DECL
} TypeTable;

static TypeRef table_add(TypeTable* table, const char* name, TypeUnderlying type) {
	if (table->count == TYPETABLE_CAP) {
		fprintf(stderr, "TypeTable: out of slots\n");
		abort();
	}

	size_t ref = table_hash(name) % TYPETABLE_CAP;
	for (;;) {
		if (table->entries[ref].name == NULL) {
			table->entries[ref].name = name;
			table->entries[ref].type = type;
			break;
		}
		ref = (ref+1) % TYPETABLE_CAP;
	}
	return &table->entries[ref];
}

static void table_init(TypeTable* table) {
	memset(table, 0, sizeof(TypeTable));

	#define MAKE_TYPE(name, type, data_) table->type_##name = table_add(table, #name, (TypeUnderlying){.tag=TYPE_##type, .data=data_, .child=NULL})

	MAKE_TYPE(i0, INT, 0);
	MAKE_TYPE(i8, INT, 8);
	MAKE_TYPE(i16, INT, 16);
	MAKE_TYPE(i32, INT, 32);
	MAKE_TYPE(i64, INT, 64);

	MAKE_TYPE(u8, UINT, 8);
	MAKE_TYPE(u16, UINT, 16);
	MAKE_TYPE(u32, UINT, 32);
	MAKE_TYPE(u64, UINT, 64);
	MAKE_TYPE(usize, UINT, 1);

	MAKE_TYPE(f0, FLOAT, 0);
	MAKE_TYPE(f16, FLOAT, 16);
	MAKE_TYPE(f32, FLOAT, 32);
	MAKE_TYPE(f64, FLOAT, 64);
	MAKE_TYPE(f80, FLOAT, 80);
	MAKE_TYPE(f128, FLOAT, 128);

	MAKE_TYPE(bool, BOOL, 0);
	MAKE_TYPE(void, VOID, 0);
	MAKE_TYPE(type, TYPE, 0);

	table->type_str = table_add(table, TYPE_ANON, (TypeUnderlying){.tag=TYPE_SLICE, .data=0, .child=table->type_u8});

	#undef MAKE_TYPE
}

static TypeRef table_get(TypeTable* table, const char* name) {
	const size_t init_ref = table_hash(name) % TYPETABLE_CAP;
	size_t ref = init_ref;
	bool first = true;
	for (;;) {
		if (!first && ref == init_ref) {
			return NULL;
		}

		if (strcmp(table->entries[ref].name, table) == 0) {
			return &table->entries[ref];
		}

		ref = (ref+1) % TYPETABLE_CAP;
		first = false;
	}
}
