<p align="center">
  <img src="assets/logo_transparent.png" alt="Acus logo" width="180">
</p>

# Acus Sugar

Acus Sugar is a layer of syntactic sugar sprinkled on top of the Acus API. It provides a programming-language-like DSL embedded in C++23, using templates, operator overloading and macros to expose variables, expressions, functions, loops, conditionals, arrays, strings, structs, pointers, globals, recursion, I/O and a small standard library.

> [!NOTE]
> Acus Sugar is under active development. The syntax and standard-library API may still change.

## Minimal C++ example

Include the Sugar umbrella header, then import the Sugar namespace:

```cpp
#include <iostream>

#include <acus/sugar/sugar.h>

using namespace acus::sugar;

int main() try {
  program_("hello");
  {
    main_() {
      println("Hello, World!");
      return_;
    };
  }
  endProgram();

  std::cout << generateBrainfuck("hello");
}
catch (std::exception const &e) {
  std::cerr << e.what() << '\n';
  return 1;
}
```

Calling this C++ program constructs the Acus program and writes the generated Brainfuck source to standard output.

## Using `acs`

A simple Python script, `acs`, is provided to remove the C++ boilerplate. It accepts a source file containing only the body between `program_()` and `endProgram()`, constructs the surrounding C++ program, compiles it and runs it to produce Brainfuck.

The Hello World example can therefore be reduced to:

```cpp
// hello.acs

main_() {
  println("Hello, World!");
  return_;
};
```

After installing `acs` to your path:

```sh
acs hello.acs -o hello.bf
```

## Starting a program

### Basic structure

A program is opened with `program_()` and finalized with `endProgram()`:

```cpp
program_("example", "main");
{
  // Globals and functions
}
endProgram();
```

The second argument names the entry function and defaults to `"main"`. After the program has been finalized, retrieve its generated source with:

```cpp
std::string bf = generateBrainfuck("example");
```

These surrounding steps are added automatically when using `acs`.

### Defining and declaring functions

A function is introduced with `function_`. The signature is written using ordinary C++ function-type syntax. The example below defines a function that takes two `u16` values and returns a `u16`:

```cpp
auto add =
  function_<u16(u16, u16)>("add", "a", "b") | define {
    return_(var_("a") + var_("b"));
  };
```

The definition returns a typed function handle:

```cpp
let_<u16>("sum") = add(10, 20);
```

The pipe followed by `define` finalizes the builder and opens the function body. The closing brace must be followed by a semicolon.

A function that is used before its definition can first be declared:

```cpp
auto add =
  function_<u16(u16, u16)>("add", "a", "b") | declare;
```

A matching definition must appear before the program is finalized.

For the common `void main()` entry function, Sugar provides `main_()`:

```cpp
main_() {
  println("Hello");
  return_;
};
```

An explicit `return_` is required on every reachable execution path.

## Types and variables

### Declaring and using variables

Declare a local variable with `let_`:

```cpp
let_<u8>("x");
let_<u8>("y") = 42;
```

Refer to a declared local or global with `var_`:

```cpp
var_("x") = var_("y") + 1;
```

Both functions return an `Expr` handle, which can be stored in a normal C++ variable:

```cpp
auto x = (let_<u8>("x") = 10);
auto y = var_("y");

x += y;
println(x);
```

The C++ variable is only a handle to data or an expression in the generated Acus program. It does not contain the runtime value itself.

### Integer types

| Type | Description |
|---|---|
| `u8` | Unsigned 8-bit integer |
| `s8` | Signed 8-bit integer |
| `u16` | Unsigned 16-bit integer |
| `s16` | Signed 16-bit integer |

```cpp
let_<u8>("small") = 200;
let_<s16>("signedValue") = -1000;
```

Integer arithmetic follows the fixed-width semantics of the selected type.

### Strings

`String<N>` stores a null-terminated string with room for at most `N` characters:

```cpp
let_<String<20>>("name") = "Ada";
println(var_("name"));
```

The capacity is part of the type. A `String<10>` and a `String<20>` are different Sugar types, although supported operations can compare strings of different capacities.

Strings support constant and runtime indexing:

```cpp
var_("name")[0] = 'E';

let_<u8>("i") = 1;
put(var_("name")[var_("i")]);
```

### Arrays

```cpp
using Scores = Array<u8, 4>;

let_<Scores>("scores") = Scores{10, 20, 30, 40};
println(var_("scores")[2]);

let_<u8>("i") = 1;
var_("scores")[var_("i")] = 99;
```

Arrays may contain any supported Sugar type, including strings, structs and other arrays:

```cpp
using NameList = Array<String<10>, 5>;
using Matrix = Array<Array<u8, 4>, 3>;
```

### User-defined types

Structs are described entirely at compile time:

```cpp
using Person = Struct<"Person",
  Field<"name", String<20>>,
  Field<"age", u8>
>;
```

They can be initialized, assigned and accessed by field name:

```cpp
let_<Person>("person") = Person{"Ada", 36};

var_("person").field("age") += 1;
println(var_("person").field("name"));
```

Struct fields may themselves contain arrays, strings or other structs. Equality and clearing are composed recursively from the operations supported by the field types.

### Pointers

```cpp
let_<u8>("value") = 10;
let_<Ptr<u8>>("p") = &var_("value");

*var_("p") = 20;
println(var_("value"));
```

Pointer arithmetic and pointers to aggregate elements are supported where the underlying type rules allow them:

```cpp
using Values = Array<u8, 5>;

let_<Values>("values") = Values{1, 2, 3, 4, 5};
let_<Ptr<u8>>("p") = &var_("values")[0];

println(*var_("p"));
++var_("p");
println(*var_("p"));
```

## Operators

The following operators use their familiar C/C++ meanings:

```text
+  -  *  /  %
<  <=  >  >=  ==  !=
&&  ||  !
=  +=  -=  *=  /=  %=
++  --
```

Unary address-of and dereference use `&` and `*`:

```cpp
&var_("x")
*var_("p")
```

Cast an expression with:

```cpp
let_<u16>("wide") = var_("x").cast<u16>();
```

> [!IMPORTANT]
> Sugar's `&&` and `||` do not short-circuit. Both operands are always evaluated while constructing the generated operation.

## Control flow

Sugar control-flow constructs resemble their C++ counterparts, but use suffixed keyword-like macros. Braces are mandatory, and each construct ends with a semicolon.

### Conditionals

```cpp
if_(var_("x") < 10) {
  println("small");
}
else_ {
  println("large");
};
```

The `else_` block is optional:

```cpp
if_(var_("x") == 0) {
  println("zero");
};
```

### While loops

```cpp
while_(var_("x") > 0) {
  println(var_("x"));
  --var_("x");
};
```

### For loops

```cpp
for_(let_<u8>("i") = 0, var_("i") < 10, ++var_("i")) {
  println(var_("i"));
};
```

### Break and continue

```cpp
for_(let_<u8>("i") = 0, var_("i") < 20, ++var_("i")) {
  if_(var_("i") == 10) {
    break_;
  };

  if_(var_("i") % 2 == 0) {
    continue_;
  };

  println(var_("i"));
};
```

## C++ as a metaprogramming language

Acus Sugar is embedded in C++, so there are two distinct kinds of control flow.

An ordinary C++ loop runs while the Brainfuck program is being constructed:

```cpp
for (size_t i = 0; i != 3; ++i) {
  println("This statement is emitted three times");
}
```

A Sugar loop becomes runtime control flow in the generated Brainfuck program:

```cpp
for_(let_<u8>("i") = 0, var_("i") < 3, ++var_("i")) {
  println(var_("i"));
};
```

The same distinction applies to `if` versus `if_`, and `while` versus `while_`. Ordinary C++ is useful for generating repeated constant-index code, defining types, selecting implementations and organizing a larger frontend.

## Globals

Globals are declared at program scope with `global_` and referenced through `var_` inside functions:

```cpp
using MyString = String<20>;
global_<MyString>("message");

main_() {
  var_("message") = "Hello";
  println(var_("message"));
  return_;
};
```

## Input and output

The basic I/O functions are available directly in `acus::sugar`:

```cpp
print('A');
print(123);
print(var_("value"));

println("Hello");
println(var_("value"));
println();

read(var_("character"));
put(var_("character"));
```

| Function | Description |
|---|---|
| `print(value)` | Prints an integer in decimal notation or prints a null-terminated string. |
| `println(value)` | Calls `print(value)` and appends a newline. |
| `println()` | Prints only a newline. |
| `read(destination)` | Reads one Brainfuck input byte into a one-cell destination. |
| `put(value)` | Writes the raw byte representation of a value without decimal formatting. |

## Standard-library functions

The complete Sugar interface is included by `<acus/sugar/sugar.h>`. Its public standard-library catalogue is intentionally collected in `include/acus/sugar/sugar_std.h`, which can be inspected to see the available callable objects and their signatures in one place.

Library functions are stateless callable objects. Operations that naturally belong to one concrete string or array type are exposed through that type:

```cpp
using Line = String<40>;
...
let<u8>("len") = Line::length(var_("line"));
let<u8>("pos") = Line::find_char(var_("line"), ':');

using Lines = Array<Line, 5>;
...
let<u8>("x") = Lines::contains(var_("lines"), var_("line"));
```
Free-standing operations remain grouped into namespaces such as `io`, `algorithm`, `ascii`, `string`, `math` and `ansi`:

```cpp
using Line = String<40>
let<Line>("line") = io::read_line<Line>();
let<u16>("x") = math::sqrt<u16>(value);
```

A returned `Expr` can be stored in a normal C++ variable. That variable is only a handle to the generated Acus expression or storage:

```cpp
auto position = Line::find_char(text, ':');
println(position);
```

### Inline and outlined use

By default, a library implementation is emitted inline at the call site:

```cpp
using Line = String<40>;

auto line = io::read_line<Line>();
println(Line::length(line));
```

Most non-mutating functions can instead be outlined once as an ordinary Acus function:

```cpp
using Line = String<40>;
using Lines = Array<Line, 5>;

auto read_line = io::read_line<Line>.outline();
auto length = Line::length.outline();
auto contains_line = Lines::contains.outline();
auto parse_int = Line::to_int<s16>.outline();

main_() {
  let_<Line>("line") = read_line();
  let_<Lines>("lines");
  println(length(var_("line")));
  println(parse_int(var_("line")));
  println(contains_line(var_("lines"), var_("line")));
  return_;
};
```

Outlining can substantially reduce generated program size when a helper is called repeatedly, but introduces function-call and argument-copying overhead. Functions marked **inline-only** modify caller-owned storage and do not provide `.outline()`.

### Generic operations

Library algorithms are enabled by the operations supported by their types rather than being restricted to integers. For example:

- strings of different capacities can be compared lexicographically;
- arrays of strings can be sorted;
- structs are assignable when their types match;
- structs are equality-comparable when all their fields are equality-comparable;
- `algorithm::clear` recursively clears integers, strings, arrays and structs;

This allows nested types to compose naturally:

```cpp
using Person = Struct<"Person",
  Field<"name", String<10>>,
  Field<"age", u8>
>;

using People = Array<Person, 5>;
auto find_person = People::find.outline();

auto people = let_<People>("people");
auto person = let_<Person>("person");
...
println(find_person(people, person));
algorithm::clear<People>(people);
```

### Library reference

Unless marked **inline-only**, a function can also be used through `.outline()`.

#### General algorithms

| Function              | Signature        | Description                                                                     | Use         |
|-----------------------|------------------|---------------------------------------------------------------------------------|-------------|
| `algorithm::clear<T>` | `(T) -> void`    | Clears a value according to its type. Works recursively for arrays and structs. | Inline-only |
| `algorithm::swap<T>`  | `(T, T) -> void` | Exchanges two assignable values.                                                | Inline-only |

#### Input

| Function                    | Signature          | Description                                                | Use                |
|-----------------------------|--------------------|------------------------------------------------------------|--------------------|
| `io::read_line<StringType>` | `() -> StringType` | Reads a line into the supplied fixed-capacity string type. | Inline or outlined |

For example:

```cpp
using Line = String<80>;
auto read_line = io::read_line<Line>.outline();
```

#### ASCII

| Function                 | Signature    | Description                                                                       |
|--------------------------|--------------|-----------------------------------------------------------------------------------|
| `ascii::is_digit`        | `(u8) -> u8` | Tests for `'0'` through `'9'`.                                                    |
| `ascii::is_alpha`        | `(u8) -> u8` | Tests for an ASCII letter.                                                        |
| `ascii::is_alphanumeric` | `(u8) -> u8` | Tests for an ASCII letter or digit.                                               |
| `ascii::is_lower`        | `(u8) -> u8` | Tests for a lowercase ASCII letter.                                               |
| `ascii::is_upper`        | `(u8) -> u8` | Tests for an uppercase ASCII letter.                                              |
| `ascii::is_whitespace`   | `(u8) -> u8` | Tests for the standard ASCII whitespace characters.                               |
| `ascii::to_lower`        | `(u8) -> u8` | Converts an uppercase ASCII letter to lowercase and leaves other bytes unchanged. |
| `ascii::to_upper`        | `(u8) -> u8` | Converts a lowercase ASCII letter to uppercase and leaves other bytes unchanged.  |

All ASCII functions can be used inline or outlined.

#### String members

For a concrete type such as:

```cpp
using Text = String<40>;
using Word = String<10>;
```

the following library objects are available on `Text`:

| Function                      | Signature                 | Description                                                                  | Use                |
|-------------------------------|---------------------------|------------------------------------------------------------------------------|--------------------|
| `Text::length`                | `(Text) -> u8/u16`        | Returns the number of characters before the null terminator.                 | Inline or outlined |
| `Text::find_char`             | `(Text, u8) -> u8/u16`    | Returns the first matching character index, or `Text::Size` when absent.     | Inline or outlined |
| `Text::starts_with<Other>`    | `(Text, Other) -> u8`     | Tests whether the string begins with another string.                         | Inline or outlined |
| `Text::ends_with<Other>`      | `(Text, Other) -> u8`     | Tests whether the string ends with another string.                           | Inline or outlined |
| `Text::find_str<Other>`       | `(Text, Other) -> u8/u16` | Returns the first matching substring index, or `Text::Size` when absent.     | Inline or outlined |
| `Text::contains<Other>`       | `(Text, Other) -> u8`     | Tests whether another string occurs within the string.                       | Inline or outlined |
| `Text::append_to_copy<Other>` | `(Text, Other) -> Text`   | Returns a copy with as much of the second string appended as fits.           | Inline or outlined |
| `Text::to_int<Int, Base>`     | `(Text) -> Int`           | Parses an integer in base 2 through 36, stopping at the first invalid digit. | Inline or outlined |
| `Text::append<Other>`         | `(Text, Other) -> void`   | Appends in place, truncating at the destination capacity.                    | Inline-only        |

Examples:

```cpp
auto find_word = Text::find_str<Word>.outline();
auto parse_hex = Text::to_int<u16, 16>.outline();

if_(Text::starts_with<Word>(text, word)) {
  println("prefix");
};
```

The index result uses `u8` for small capacities and `u16` for larger capacities.

#### Free-standing string functions

Functions for which neither string is a natural receiver, or which construct a new string, remain in `namespace string`:

| Function                      | Signature                    | Description                                                          | Use                |
|-------------------------------|------------------------------|----------------------------------------------------------------------|--------------------|
| `string::compare<Lhs, Rhs>`   | `(Lhs, Rhs) -> s8`           | Lexicographically compares strings of possibly different capacities. | Inline or outlined |
| `string::from_int<Int, Base>` | `(Int) -> String<Int::Bits>` | Converts an integer to base 2 through 36.                            | Inline or outlined |

```cpp
auto order = string::compare<String<10>, String<20>>(lhs, rhs);
auto format_hex = string::from_int<u16, 16>.outline();
```

#### Array members

Array algorithms are attached to the complete array type, so the element type and length do not need to be repeated:

```cpp
using Values = Array<u8, 5>;
using Names = Array<String<10>, 5>;

auto find_value = Values::find.outline();
auto sort_names = Names::sort;
```

For arrays shorter than 256 elements, index-returning functions use `u8`; otherwise they use `u16`. `find` returns `N` when no element matches. `min`, `max`, `min_index` and `max_index` require a non-empty array.

| Function               | Signature                      | Description                                                           | Use                |
|------------------------|--------------------------------|-----------------------------------------------------------------------|--------------------|
| `ArrayType::fill`      | `(ArrayType, T) -> void`       | Assigns the supplied value to every element.                          | Inline-only        |
| `ArrayType::find`      | `(ArrayType, T) -> u8/u16`     | Returns the first matching index, or `ArrayType::Size` when absent.   | Inline or outlined |
| `ArrayType::contains`  | `(ArrayType, T) -> u8`         | Tests whether an equal element occurs in the array.                   | Inline or outlined |
| `ArrayType::equal`     | `(ArrayType, ArrayType) -> u8` | Tests corresponding elements for equality.                            | Inline or outlined |
| `ArrayType::sum`       | `(ArrayType) -> T`             | Adds all elements using the normal overflow semantics of `T`.         | Inline or outlined |
| `ArrayType::min`       | `(ArrayType) -> T`             | Returns the first minimum value.                                      | Inline or outlined |
| `ArrayType::max`       | `(ArrayType) -> T`             | Returns the first maximum value.                                      | Inline or outlined |
| `ArrayType::min_index` | `(ArrayType) -> u8/u16`        | Returns the index of the first minimum value.                         | Inline or outlined |
| `ArrayType::max_index` | `(ArrayType) -> u8/u16`        | Returns the index of the first maximum value.                         | Inline or outlined |
| `ArrayType::count`     | `(ArrayType, T) -> u8/u16`     | Counts elements equal to the supplied value.                          | Inline or outlined |
| `ArrayType::sort`      | `(ArrayType) -> void`          | Sorts in ascending order using the element type's ordering operation. | Inline-only        |
| `ArrayType::is_sorted` | `(ArrayType) -> u8`            | Tests whether the array is in nondecreasing order.                    | Inline or outlined |

Only operations supported by the element type are usable. For example, `Array<Person, 5>::find` is available when `Person` supports equality, while sorting additionally requires an ordering operation.

Array algorithms may select either constant-index unrolling or runtime-index loops according to the configuration in `sugar_config.h`; this does not change their public call syntax.

#### Mathematics

| Function               | Signature                | Description                                                          |
|------------------------|--------------------------|----------------------------------------------------------------------|
| `math::min<Int>`       | `(Int, Int) -> Int`      | Returns the smaller value.                                           |
| `math::max<Int>`       | `(Int, Int) -> Int`      | Returns the greater value.                                           |
| `math::clamp<Int>`     | `(Int, Int, Int) -> Int` | Restricts a value to an inclusive minimum and maximum.               |
| `math::abs<Int>`       | `(Int) -> Int`           | Returns the absolute value.                                          |
| `math::pow<Int>`       | `(Int, Int) -> Int`      | Raises a value to an integer power.                                  |
| `math::sqrt<Int>`      | `(Int) -> Int`           | Returns the integer square root.                                     |
| `math::log<Base, Int>` | `(Int) -> u8`            | Returns the integer logarithm for a compile-time base of at least 2. |
| `math::log2<Int>`      | `(Int) -> u8`            | Returns the integer base-2 logarithm.                                |
| `math::log10<Int>`     | `(Int) -> u8`            | Returns the integer base-10 logarithm.                               |
| `math::gcd<Int>`       | `(Int, Int) -> Int`      | Returns the greatest common divisor.                                 |

All mathematics functions can be used inline or outlined.

### ANSI terminal screens

`ansi::Screen<Width, Height, Left, Top>` describes a fixed rectangular region of an ANSI-compatible terminal. `Left` and `Top` default to 1 and use ANSI's one-based terminal coordinates; coordinates passed to the methods are zero-based within the screen.

```cpp
using Screen = ansi::Screen<40, 20>;

Screen::begin();
Screen::clear();
Screen::put(5, 2, '@');
Screen::write<13>(3, 5, "Hello, world!");
Screen::end();
```

| Member             | Signature                     | Description                                                    |
|--------------------|-------------------------------|----------------------------------------------------------------|
| `Screen::begin`    | `() -> void`                  | Hides the terminal cursor before drawing.                      |
| `Screen::move_to`  | `(u8, u8) -> void`            | Moves to a zero-based coordinate inside the configured region. |
| `Screen::put`      | `(u8, u8, u8) -> void`        | Moves to a coordinate and writes one raw character.            |
| `Screen::write<N>` | `(u8, u8, String<N>) -> void` | Moves to a coordinate and prints a string horizontally.        |
| `Screen::clear`    | `() -> void`                  | Clears only the configured region and returns to `(0, 0)`.     |
| `Screen::end`      | `() -> void`                  | Restores the cursor and moves below the configured region.     |

Screen operations write ANSI escape sequences directly and do not maintain a framebuffer. The caller is responsible for keeping coordinates within the configured dimensions. Screen operations can be outlined in the same way as other library functions:

```cpp
auto put_screen = Screen::put.outline();
```

## Examples

The `examples` directory contains small programs demonstrating both the core Acus API and Sugar. The Sugar-focused examples include:

| Example                | Highlights                                                                       |
|------------------------|----------------------------------------------------------------------------------|
| `sugar_hello_world.cc` | Minimal program structure and formatted output.                                  |
| `sugar_text.cc`        | Line input, outlining, ASCII conversion, string length and integer conversion.   |
| `sugar_arrays.cc`      | Arrays of strings, sorting, searching and recursive clearing.                    |
| `sugar_structs.cc`     | Structs, arrays of structs, generic equality, `find` and `clear`.                |
| `pong.cc`              | Functions, globals, runtime input and ANSI screen drawing in a complete program. |

Build the examples with:

```sh
make examples
```

## License

Acus and Acus Sugar are licensed under the GNU General Public License v3.0 or later. See `LICENSE` for details.
