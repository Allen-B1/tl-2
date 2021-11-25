#ifndef _TYPES_H
#define _TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arrlist.h>
#include "../utils.h"

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

	TYPE_FUNC, // .data = pointer TypeFunc*, .child = return type
} TypeTag;

#define TYPE_MUT 0x2
#define TYPE_OPT 0x1

typedef size_t TypeRef;
//extern const char* const TYPE_ANON;

typedef struct {
	TypeTag tag;
	uint64_t data;
	TypeRef child;
} Type;

typedef struct {
	const char* name;
	TypeRef type;
} TypeFieldsField;

typedef struct {
	size_t len;
	ArrList /*TypeFieldsField*/ fields;
} TypeFields;

typedef struct {
	bool varardic;
	TypeRef ret_type;
	ArrList /*TypeRef*/ arg_types;
} TypeFuncData;

typedef struct {
	const char* name;
	Type type;
} TypeEntry;

typedef struct TypeTable TypeTable;

bool type_is_eq(TypeTable* table, TypeRef from, TypeRef to);

// auto-coerce
bool type_can_coerce(TypeTable* table, TypeRef from, TypeRef to);

bool type_can_cast(TypeTable* table, TypeRef from, TypeRef to);

bool type_is_runtime(TypeTable* table, TypeRef type);


enum {
	TYPEREF_VOID,
	TYPEREF_BOOL,

	TYPEREF_I8,
	TYPEREF_I16,
	TYPEREF_I32,
	TYPEREF_I64,
	TYPEREF_ISIZE, // c::intptr_t

	TYPEREF_U8,
	TYPEREF_U16,
	TYPEREF_U32,
	TYPEREF_U64,
	TYPEREF_USIZE, // c::uintptr_t

	TYPEREF_F16,
	TYPEREF_F32,
	TYPEREF_F64,
	TYPEREF_F64X,

	TYPEREF_GENERIC_INT,
	TYPEREF_GENERIC_FLOAT,

	TYPEREF_TYPE,
	TYPEREF_STR, // []const u8
};

#define TYPEREF_ERR SIZE_MAX

TypeEntry* typetable_get(const TypeTable* table, TypeRef ref);
TypeRef typetable_add(TypeTable* table, const char* name, Type type);
void typetable_init(TypeTable* table);

struct TypeTable {
	ArrList entries;
};

#endif