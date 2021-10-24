design: C  
 + namespaces/methods  
 + optionals & slices & `into`   
 + immutability by default  
 + better string handling  
 + better stdlib?  
`.` operator: arrays, slices, `struct`, `union` only!

# Language
## Types
### Primitive Types
 - `void` - contains one value, `null`
 - `bool` - `true` or `false`
 - `i8, i16, i32, i64, u8, u16, u32, u64, usize` - signed/unsigned integers
 - `f16, f32, f64, f80, f128` - floating point. `f80` and `f128` fall back to `f80` or `f64` if they're not available.
 - `varargs` - C's `va_list`
 - `type` - represents a type. only available during compile time.

All numeric types have associated constants `T::min` and `T::max` describing the minimum and maximum value
of that type.
  

### Composite Types
#### Arrays
 - `[N]T`- array of N `T`s

An array is a contiguous location of memory. The pointer to 
an array is equal to the pointer to the first element
of that array; in other words

```
(*void)(&arr) == (*void)(&arr[0])
```

Arrays can be indexed with bracket syntax (`arr[idx]`). Arrays cannot be passed
by value as a function parameter.

#### Pointers & Slices
 - `*T` - immutable pointer to `T`
 - `*mut T` - mutable pointer to `T`
 - `[*]T` - immutable slice that points to `T`
 - `[*]mut T` - mutable slice that points to `T`

Additionally, the `restrict` keyword can be used right before `T` that behaves in the same way
that C's `restrict` does.

Slices are built with
`x[start..end]` syntax, which indexes array, pointer, or slice `x` from \[`start`, `end`).
Slices can automatically coerce into pointers
(`[*]T` => `*T`, `[*]mut T` => `*mut T`, `*T`).

`slice.len` returns the length of a slice.

Pointers and slices can be made optional by adding a `?` in front; `void` can
convert to optional types.

#### Structs
Structs represent a contiguous memory location, laid out sequentially.
Fields are laid out in memory in the order they are declared.
The pointer to a struct value is equal to the pointer to the its first field.

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
coerced into the pointer to the type of the `into` field, as described in the Type Conversions section.

```rust
type ColorA = struct {
	into color Color; // keyword into
	alpha u32;
};

let green = ColorA { Color{0, 255, 0}, 128 };
let clr *Color = &green; // == &green.color
```

As a special case, the last member of a struct maybe an unknown-sized array, called a flexible array member (FAM). This, a struct that has a FAM can never be stack-allocated. The size of the array is 0 when factoring into `sizeof`.

```rust
type Buffer = struct {
	cap usize;
	len usize;
	data []u8;
};

let buf Buffer; // error!
let buf *Buffer = mem::alloc(sizeof(Buffer) + 5 * sizeof(u8));
```

#### Unions
A union type can hold one field at a time. The pointer
to a union value is equal to the pointer to any of its fields.

```rust
type A = union {
	i i32;
	f f32;
	s [*]u8;
};

let int = A{.i=3};
let float = A{.f=2.56};
let str = A{.s="a string"};
```

#### Enums
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

### Type Conversions
#### Coercion
Type F can coerce into type T iff:
 - F is equal to T
 - (num -> num) F and T are number types where all values of F exist inside of T
 - (enum -> num) F is an `enum` type or `bool` and T is an integer type that F fits inside of
 - (void -> ptr|slice) F is `void` and T is an optional type
 - (ptr -> ptr) F & T are pointer types with the same child type or a `void` child type; mutability and optionality are compatible
 - (slice -> ptr) F is a slice type and T is a pointer type with the same child type or `void` child type; mutability and optionality are compatible
 - (struct ptr -> into ptr) F is a pointer to a struct type and T is a pointer to the type of the `into` field of F

### Methods
If a function's namespace is the same as a given type `T` and 
the first argument of the function is either:

 - a pointer to `T`
 - `T`

then a value of type `T` `val` can call the function like so: `val.func_name(arg2, arg3...)`.

## Builtins
### Type Functions
#### `sizeof`
Returns the size of a given type.

```rust
func sizeof(T type) usize;
```

#### `alignof`
Returns the alignment of a given type.

```rust
func alignof(T type) usize;
```

#### `typeof`
Compile-time macro function.

```rust
func typeof(val) type;
```

### Varardic Functions
#### `varargs::next`
```rust
func varargs::next(varargs args, T type) T;
```

#### `varargs::copy`
```rust
func varargs::copy(varargs args) varargs;
```

#### `varargs::free`
```rust
func varargs::free(varargs args);
```

This should be called once for every varardic parameter passed, as well as every call to `varargs::copy`.

### Type Constants
For every signed integer type `t`, there exist two constants:
 - `t::MAX`
 - `t::MIN`
 
For every unsigned integer type `t`, there exists one constant:
 - `t::MAX`

For every floating point type `t` there exists the following:
 - `t::MIN` - minimum representable positive value
 - `t::MAX` - maximum representable value
 - `t::EPSILON` - minimum representable value greater than 1, minus 1

## Declarations
### Namespaces
Symbol names can contain `::`.

### `const`
Constants are compile-time constants. 

```rust
const string [*]u8 = "i am a string";
const pi f64 = 3.1415926535;
const e f32 = 2.71;

// next two statements are identical
const ID type = u32;
type ID = u32;
```

### `let`
Let declares an automatic storage duration variable. `let` statements cannot occur in the global scope.

```rust
let id ID = 3;

// for type inference, use the auto keyword
let mut next auto = id + 1;
next += 1;
```

### `static`
A `static` statement has the same syntax as a `let` declaration, but has the static storage duration.

```rust
static mut buf [64]u8;
return buf[..5];
```

### `extern`
An `extern` declaration declares a variable but does not define it.

### `func`
Basically the same as C.

```rust
func add_one(n u32) u32 {
	return n + 1;
}

// if no return type is specified,
// `void` is implied.
func say_hi(name [*]u8) {
	io::print("hello, ");
	io::print(name);
	// return null;
}

// inline functions are inlined at compile-time.
func inline dne() {
	abort();
	return null;
}

// varardic arguments
func lol(arg i32, args...) {
	// args initialized with va_start

	let age = args.next(i8);
	let name = args.next([*]u8);

	let args_copy = args.copy();

	// va_end called
}
```

## Control Flow
### `if`
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

### `switch`
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

### `for`
Same as Go. Includes labeled `break` and `continue` statements.

### `goto`
Same as C.

# Preprocessor
## `include`
Basically C's `#include`. If there are no quotes, the file is looked for in the
given include paths (`-I`). If there are quotes, the string is interpreted
as a relative path to the current file.

```c
#include <mem>
#include "./file.tl"
```

## `define`, `undef`
Declares a macro. A macro is a set of tokens that expand to another set of tokens.

### Constant Macros
```c
#define RANDOM rand::next()
#define if for
```

### Function Macros
```c
#define COOLMACRO($kw) $kw A { *(*mut u32)0xc001 = 0xdab; }
// `kw` directly substituted in

#define FASTMOD($(a), $(b) ($a & ($b - 1))
// `$a` and `$b` substituted in with parenthesis.

#define FASTSQRT($(a u32)) ($a << 2)

#define R32($(a) / $(b)) r32::new($a, $b)
// RAT(3 / 5) expands to r32::new((3), (5))

#define SET{$(T type); $(a u32 ,)} set::new(sizeof($T), $a::len, []$T{$a})
// SET{u32; 1, 2, 6, 3} expands to set::new(sizeof((u32)), 4, []u32{1, 2, 6, 3})

#undef COOLMACRO
// oh no :((
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

