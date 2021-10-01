design: C  
 + namespaces/methods  
 + optionals & slices & `into`   
 + immutability by default  
 + better string handling  
 + better stdlib?  
`.` operator: arrays, slices, `struct`, `union` only!

# Types
## Types of Types
### Primitive Types
 - `void` - contains one value, `null`
 - `bool` - `true` or `false`
 - `i8, i16, i32, i64, u8, u16, u32, u64, usize` - signed/unsigned integers
 - `f16, f32, f64, f80, f128` - floating point. `f80` and `f128` fall back to `f80` or `f64` if they're not available.
 - `varargs` - C's `va_list`
 - `type` - represents a type. only available during compile time.

### Arrays
 - `[N]T`- array of N `T`s

An array is a contiguous location of memory. The pointer to 
an array is equal to the pointer to the first element
of that array; in other words

```
(*void)(&arr) == (*void)(&arr[0])
```

Arrays can be indexed with bracket syntax (`arr[idx]`). Arrays cannot be passed
by value as a function parameter.

### Pointers & Slices
 - `*T` - immutable pointer to `T`
 - `*mut T` - mutable pointer to `T`
 - `[]T` - immutable slice that points to `T`
 - `[]mut T` - mutable slice that points to `T`

Slices are built with
`x[start..end]` syntax, which indexes array, pointer, or slice `x` from \[`start`, `end`).
Slices can automatically coerce into pointers
(`[]T` => `*T`, `[]mut T` => `*mut T`, `*T`). 

`slice.len` returns the length of a slice.

Pointers and slices can be made optional by adding a `?` in front; `void` can
convert to optional types.

### Structs
Structs represent a contiguous memory location, laid out sequentially.
Fields are laid out in memory in the order they are declared.
The pointer to the struct is equal to the pointer to the first field.

```rust
type Color = struct {
	red u32;
	green u32;
	blue u32;
};

let yellow = Color { 255, 255, 0 };
let sky = Color { .red = 3, .green = 3, .blue = 200 };
let red u32 = sky.red;
```

The `into` keyword can be used in the first field
of a struct, which allows pointers to the struct to be automatically
coerced into the pointer to the type of the `into` field, as described in the next section.

```rust
type ColorA = struct {
	into color Color; // keyword into
	alpha u32;
}

let green = ColorA { Color{0, 255, 0}, 128 };
let clr *Color = &green; // == &green.color
```

## Unions
A union type can hold one field at a time. The pointer
to a union value is equal to the pointer to any of its fields,
and can be automatically coerced.

```rust
type A = union {
	i i32;
	f f32;
	s []u8;
};

let int = A{.i=3};
let float = A{.f=2.56};
let str = A{.s="a string"};
```

## Enums
Every member declared inside of an `enum` becomes a `const` declaration.

```rust
type Side = enum u8 {
	TOP,
	BOTTOM,
	LEFT,
	CENTER
};

let a = Side::TOP;
let b = Side::BOTTOM;
```

## Associated `const`s declared
 - `T::size` - size of type T
 - `T::align` - alignment of type T
 - `T::max` - maximum value of type T
 - `T::min` - minimum value of type T

## "Methods"
If a function's namespace is the same as a given type `T` and 
the first argument of the function is either:

 - a pointer to `T`
 - `T`

then a value of type `T` `val` can call the function like so: `val.func_name(arg2, arg3...)`.

## `typeof`
Compile-time macro function.

```rust
macro typeof(val);
```

# Declarations
## Namespaces
Symbol names can contain `::`.

## `const`
Constants are compile-time constants. 

```rust
const string = "i am a string";
// type = generic float
const pi = 3.1415926535;
// type = f32
const e f32 = 2.71;

// next two statements are identical
const ID type = u32;
type ID = u32;
```

## `let`
Let declares a runtime constant or variable.

```rust
let id ID = 3;
let mut next = id + 1;
next += 1;
```

## `func`
Basically the same as C.

```rust
func add_one(n u32) u32 {
	return n + 1;
}

// if no return type is specified,
// `void` is implied.
func say_hi(name []u8) {
	io::print("hello, ");
	io::print(name);
	// return null;
}

// inline functions are inlined at compile-time.
inline func dne() {
	abort();
	return null;
}

// varardic arguments
func lol(arg i32, args...) {
	// args initialized with va_start

	let age = args.next(i8);
	let name = args.next([]u8);

	let args_copy = args.copy();

	// va_end called
}
```

# Control Flow
## `if`
Basically the same as C.

```rust
if condition {
	// do stuff
} else if condition2 {
	// do stuff
} else {
	// do stuff
}
```

## `switch`
Basically the same as C.

```rust
switch val {
	case 3:
		// stuff
	case 4:
		// stuff
	case 5..10:
		// stuff
	else:
}
```

## `for`
Same as Go.


# Preprocessor
## `include`
Basically C's `#include`. If there are no quotes, the file is looked for in the
given include paths (`-I`). If there are quotes, the string is interpreted
as a relative path to the current file.

```rust
#include mem
#include "./file.tl"
```

## `define`, `undef`
Declares a macro. Macros, like any symbol, can have a namespace.

```rust
// Expression macros
#define ADD_ONE(a i32) a + 1
#define STMT(a u32) {
	let b = ADD_ONE(a);
	io::printf("%d\n", b);
}

// Untyped
#define SIZEOF(T) T::size
```

## `ifdef`, `ifndef`, `if`, `else`, `end`
Same as c.

## Builtin Macros
```
env::IS_WINDOWS
env::IS_LINUX
env::IS_MAC
env::IS_BSD
```

