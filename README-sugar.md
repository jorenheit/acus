<p align="center">
  <img src="assets/logo_transparent.png" alt="Acus logo" width="180">
</p>

# Acus Sugar

Acus Sugar is a layer of syntactic sugar sprinkled on top of the Acus API. It provides a programming-language-like DSL embedded in C++23, using templates, operator overloading and macros to expose variables, expressions, functions, loops, conditionals, arrays, strings, structs, pointers, globals, recursion, I/O and a small standard library.

> [!NOTE]
> Acus Sugar is under active development. The syntax and standard-library API may still change.

## Learn By Example
Prefer learning by example? Skip to [the list of full examples below](#full-examples).

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

>[!NOTE]
>These steps are added automatically when using `acs`.

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
let_<u8>("len") = Line::length(var_("line"));
let_<u8>("pos") = Line::find_char(var_("line"), ':');

using Lines = Array<Line, 5>;
...
let_<u8>("x") = Lines::contains(var_("lines"), var_("line"));
```
Free-standing operations remain grouped into namespaces such as `io`, `algorithm`, `ascii`, `string`, `math` and `ansi`:

```cpp
using Line = String<40>;
let_<Line>("line") = io::read_line<Line>();
let_<u16>("x") = math::sqrt<u16>(value);
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

<details>
<summary><strong>General Algorithms</strong></summary>

| Function              | Signature        | Description                                                                     | Use         |
|-----------------------|------------------|---------------------------------------------------------------------------------|-------------|
| `algorithm::clear<T>` | `(T) -> void`    | Clears a value according to its type. Works recursively for arrays and structs. | Inline-only |
| `algorithm::swap<T>`  | `(T, T) -> void` | Exchanges two assignable values.                                                | Inline-only |

</details>

<details> <summary><strong>Input</strong></summary>

| Function                    | Signature          | Description                                                | Use                |
|-----------------------------|--------------------|------------------------------------------------------------|--------------------|
| `io::read_line<StringType>` | `() -> StringType` | Reads a line into the supplied fixed-capacity string type. | Inline or outlined |

For example:

```cpp
using Line = String<80>;
auto read_line = io::read_line<Line>.outline();
```
</details>


<details> <summary><strong>ASCII</strong></summary>

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
</details>

<details> <summary><strong>Strings</strong></summary>

#### String<N> members
For a concrete type such as:

```cpp
using Text = String<40>;
using Word = String<10>;
```

the following library objects are available on `Text`:

| Function                           | Signature                      | Description                                                                  | Use                |
|------------------------------------|--------------------------------|------------------------------------------------------------------------------|--------------------|
| `String<N>::length`                | `(String<N>) -> u8/u16`        | Returns the number of characters before the null terminator.                 | Inline or outlined |
| `String<N>::find_char`             | `(String<N>, u8) -> u8/u16`    | Returns the first matching character index, or `Text::Size` when absent.     | Inline or outlined |
| `String<N>::starts_with<Other>`    | `(String<N>, Other) -> u8`     | Tests whether the string begins with another string.                         | Inline or outlined |
| `String<N>::ends_with<Other>`      | `(String<N>, Other) -> u8`     | Tests whether the string ends with another string.                           | Inline or outlined |
| `String<N>::find_str<Other>`       | `(String<N>, Other) -> u8/u16` | Returns the first matching substring index, or `Text::Size` when absent.     | Inline or outlined |
| `String<N>::contains<Other>`       | `(String<N>, Other) -> u8`     | Tests whether another string occurs within the string.                       | Inline or outlined |
| `String<N>::append_to_copy<Other>` | `(String<N>, Other) -> Text`   | Returns a copy with as much of the second string appended as fits.           | Inline or outlined |
| `String<N>::to_int<Int, Base>`     | `(String<N>) -> Int`           | Parses an integer in base 2 through 36, stopping at the first invalid digit. | Inline or outlined |
| `String<N>::append<Other>`         | `(String<N>, Other) -> void`   | Appends in place, truncating at the destination capacity.                    | Inline-only        |

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
</details>

<details> <summary><strong>Arrays</strong></summary>
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
</details>

<details><summary><strong>Math</strong></summary>

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
</details>

<details> <summary><strong>ANSI Terminal Screen</strong></summary>

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
</details>

## Full Examples
Listed below are the complete programs from the [`tools/acs/examples`](tools/acs/examples) directory, ordered roughly from simplest to most involved. After [installing `acs`](tools/acs/README.md), copy an example into a `.acs` file and compile it to Brainfuck with:

```sh
acs example.acs -o example.bf
```

The generated Brainfuck can then be run with any compatible interpreter, including [`bfint`](tools/bfint/README.md).

<details>
<summary><strong>01 — Hello World</strong></summary>

Source: [`01_hello.acs`](tools/acs/examples/01_hello.acs)

```cpp
// A .acs file normally contains the contents of the Acus program scope.
function_<void()>("main") | define {
  println("Hello from Acus sugar!");
  return_;
};
```

</details>

<details>
<summary><strong>02 — Variables and Arithmetic</strong></summary>

Source: [`02_variables_and_arithmetic.acs`](tools/acs/examples/02_variables_and_arithmetic.acs)

```cpp
function_<void()>("main") | define {
  // Acus provides signed and unsigned 8- and 16-bit integers.
  let_<u8>("small") = 12;
  let_<s8>("temperature") = s8{-7};
  let_<u16>("wide") = var_("small").cast<u16>() * 100;

  println(var_("small") + 3);
  println(var_("temperature") - s8{5});
  println(var_("wide") / 4);
  println(var_("wide") % 7);

  var_("small") += 10;
  var_("small") *= 2;
  println(var_("small"));

  // Comparisons and logical operators produce Boolean values.
  println((var_("small") > 20) && (var_("temperature") < s8{0}));
  println((var_("small") == 44) || (var_("wide") == 0));

  return_;
};
```

</details>

<details>
<summary><strong>03 — Control Flow</strong></summary>

Source: [`03_control_flow.acs`](tools/acs/examples/03_control_flow.acs)

```cpp
function_<void()>("main") | define {
  let_<u8>("value") = 7;

  if_(var_("value") < 10) {
    println("value is small");
  } else_ {
    println("value is large");
  };

  // Print odd numbers below 10, but stop before 9.
  for_(let_<u8>("i") = 0, var_("i") < 10, ++var_("i")) {
    if_(var_("i") % 2 == 0) {
      continue_;
    };
    if_(var_("i") == 9) {
      break_;
    };
    println(var_("i"));
  };

  let_<u8>("countdown") = 3;
  while_(var_("countdown") > 0) {
    println(var_("countdown"));
    --var_("countdown");
  };

  return_;
};
```

</details>

<details>
<summary><strong>04 — Functions</strong></summary>

Source: [`04_functions.acs`](tools/acs/examples/04_functions.acs)

```cpp
// Declare a function before main so it can be called before its definition.
auto square = function_<u16(u8)>("square") | declare;
auto printRange = function_<void(u8, u8)>("printRange") | declare;

function_<void()>("main") | define {
  println(square(12));
  printRange(3, 7);
  return_;
};

function_<u16(u8)>("square", "x") | define {
  return_(var_("x").cast<u16>() * var_("x"));
};

function_<void(u8, u8)>("printRange", "first", "last") | define {
  for_(let_<u8>("i") = var_("first"), var_("i") <= var_("last"), ++var_("i")) {
    println(var_("i"));
  };
  return_;
};
```

</details>

<details>
<summary><strong>05 — Recursion</strong></summary>

Source: [`05_recursion.acs`](tools/acs/examples/05_recursion.acs)

```cpp
auto fibonacci = function_<u16(u8)>("fibonacci") | declare;

function_<void()>("main") | define {
  for_(let_<u8>("i") = 0, var_("i") < 10, ++var_("i")) {
    println(fibonacci(var_("i")));
  };
  return_;
};

function_<u16(u8)>("fibonacci", "n") | define {
  if_(var_("n") < 2) {
    return_(var_("n"));
  } else_ {
    return_(fibonacci(var_("n") - 1) + fibonacci(var_("n") - 2));
  };
};
```

</details>

<details>
<summary><strong>06 — Arrays and Structs</strong></summary>

Source: [`06_arrays_and_structs.acs`](tools/acs/examples/06_arrays_and_structs.acs)

```cpp
// Define an array-type
using Values = Array<u8, 5>;

// Define a structure containing 2 fields
using Point = Struct<"Point",
  Field<"x", u8>,
  Field<"y", u8>
>;

// Function that takes the array
auto print_array = function_<void(Values)>("print_array", "arr") | define {
  auto arr = var_("arr");
  print('[');
  for_(let_<u8>("i") = 0, var_("i") < 5, ++var_("i")) {
    print(arr[var_("i")]);
    if_(var_("i") < 4) { print(", "); };
  };
  println(']');
  return_;
};

// Function that takes the Point-type
auto print_point = function_<void(Point)>("print_point", "p") | define {
  auto p = var_("p");
  print('{');
  print(p.field("x"));
  print(", ");
  print(p.field("y"));
  println('}');
  return_;
};

// Main
function_<void()>("main") | define {
  let_<Point>("point") = Point{3, 4};
  let_<Values>("values") = Values{2, 4, 6, 8, 10};

  auto p = var_("point");
  p.field("x") += 10;
  print_point(p);

  auto arr = var_("values");
  arr[2] = 42;
  print_array(arr);

  return_;
};
```

</details>

<details>
<summary><strong>07 — Pointers</strong></summary>

Source: [`07_pointers.acs`](tools/acs/examples/07_pointers.acs)

```cpp
using Values = Array<u8, 5>;

auto setValue = function_<void(Ptr<u8>, u8)>("setValue") | declare;

function_<void()>("main") | define {
  let_<Values>("values") = Values{1, 2, 3, 4, 5};

  setValue(&var_("values")[2], 99);

  let_<Ptr<u8>>("p") = &var_("values")[0];
  for_(let_<u8>("i") = 0, var_("i") < 5, ++var_("i")) {
    println(*var_("p"));
    ++var_("p");
  };

  return_;
};

function_<void(Ptr<u8>, u8)>("setValue", "target", "value") | define {
  *var_("target") = var_("value");
  return_;
};
```

</details>

<details>
<summary><strong>08 — Globals</strong></summary>

Source: [`08_globals.acs`](tools/acs/examples/08_globals.acs)

```cpp
global_<u16>("calls");
global_<String<16>>("message");

auto greet = function_<void()>("greet") | declare;

function_<void()>("main") | define {
  var_("calls") = 0;
  var_("message") = "Hello, world!";

  greet();
  greet();
  println(var_("calls"));

  return_;
};

function_<void()>("greet") | define {
  ++var_("calls");
  println(var_("message"));
  return_;
};
```

</details>

<details>
<summary><strong>09 — Input and Strings</strong></summary>

Source: [`09_input_and_strings.acs`](tools/acs/examples/09_input_and_strings.acs)

```cpp
using NameString = String<20>;
auto read_line = io::read_line<NameString>.outline();
auto strlen = NameString::length.outline();

function_<void()>("main") | define {
  print("What is your name? ");
  auto name = read_line();

  print("Hello, ");
  println(name);

  print("Your name has ");
  print(strlen(name));
  println(" characters.");

  return_;
};
```

</details>

<details>
<summary><strong>10 — Prime Sieve</strong></summary>

Source: [`10_prime_sieve.acs`](tools/acs/examples/10_prime_sieve.acs)

```cpp
// Sieve of Eratosthenes
// Asks for N and prints all prime numbers up to and including N.

constexpr size_t MinLimit = 2;
constexpr size_t MaxLimit = 100;

using Input = String<3>;
using Flags = Array<u8, MaxLimit + 1>;

auto read_line = io::read_line<Input>.outline();
auto to_int = Input::to_int<u8>.outline();


auto read_limit = function_<u8()>("read_limit") | define {
  auto n = let_<u8>("n");
  auto valid = (let_<u8>("valid") = 0);

  while_(!valid) {
    print("Find primes up to N (2-100): ");
    n = to_int(read_line());

    if_(n < MinLimit) {
      println("N must be at least 2.");
    }
    else_ {
      if_(n > MaxLimit) {
        println("N must not exceed 100.");
      }
      else_ {
        valid = 1;
      };
    };
  };

   return_(n);
};


auto make_sieve = function_<Flags(u8)>("make_sieve", "n") | define {
  auto n = var_("n");
  auto is_prime = let_<Flags>("is_prime");

  // Initially assume that every number is prime.
  Flags::fill(is_prime, 1);
  is_prime[0] = 0;
  is_prime[1] = 0;

  // Strike out the multiples of every remaining prime.
  auto prime = (let_<u8>("prime") = 2);

  while_(prime * prime <= n) {
    if_(is_prime[prime]) {
      auto multiple = (let_<u8>("multiple") = prime * prime);

      while_(multiple <= n) {
        is_prime[multiple] = 0;
        multiple += prime;
      };
    };

    ++prime;
  };

  return_(is_prime);
};


auto print_primes =
  function_<void(Flags, u8)>("print_primes", "is_prime", "n") | define {
    auto is_prime = var_("is_prime");
    auto n = var_("n");

    println("Prime numbers:");

    auto number = (let_<u8>("number") = 2);

    while_(number <= n) {
      if_(is_prime[number]) {
        print(number);
        print(' ');
      };

      ++number;
    };

    println();
    return_;
  };


main_() {
  auto n = (let_<u8>("n") = read_limit());
  auto is_prime = (let_<Flags>("is_prime") = make_sieve(n));

  print_primes(is_prime, n);
  return_;
};
```

</details>

<details>
<summary><strong>11 — Mandelbrot Set with ANSI Screen</strong></summary>

Source: [`11_mandelbrot.acs`](tools/acs/examples/11_mandelbrot.acs)

<details><summary><strong>! Spoiler Alert !</strong></summary>

```text
              .................................:-=--=@............              
            .................................::--=@=-::..............           
         .................................:::-=@@@@@@*@:...............         
       ...............................::::::--+@@@@@@@--:::::............       
      .............................:-=%*=-+@@@@@@@@@@@#@@@--:---:..........     
    ............................:::--=@@@@@@@@@@@@@@@@@@@@@#@@@@-...........    
   ..........................:::::+@+#@@@@@@@@@@@@@@@@@@@@@@@%=-::...........   
   ...............:--::::--:::::--=*@@@@@@@@@@@@@@@@@@@@@@@@@@@--#............  
  ...............::-=*@#=+@@#==--@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*=-:............ 
 ...............::-==@@@@@@@@@@@*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*............. 
 .........:::::=*==%@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*-:............. 
 ..:::----=--===@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@-:::............. 
.:.::-*---*--=@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@=-:::..............
 ........:::::-==#@*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@=-:............. 
 ..............::--#=@@@@@@@@@@@#@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@#-:............. 
  ...............::--#%%%@@@@@+==*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@#-::............ 
  ...............::-@-:::---:::--=#@@@@@@@@@@@@@@@@@@@@@@@@@@@@*=@............  
   .................:.......:::::-=%@*@@@@@@@@@@@@@@@@@@@@@@@@=-::...........   
    ............................:::--#@@@@@@@@@@@@@@@@@@@@@%@@@@:...........    
     .............................::-@@#%=*@@@@@@@@@@@@@@@--=-+-:..........     
       ..............................::::::---=@@@@@@%--::::::............      
         ................................::::-*@@@@@@%-::...............        
           ..................................::-=*@*--:..............           
              .................................:-=--=@............ 
```

</details>


```cpp
// Mandelbrot set with progress display
//
// Renders an 80 x 24 ASCII image. The first row of the ANSI screen is
// reserved for a progress indicator.

constexpr int ImageWidth = 80;
constexpr int ImageHeight = 24;
constexpr int ScreenHeight = ImageHeight + 1;
constexpr int MaxIterations = 24;

constexpr int Scale = 64;
constexpr int XMin = -2 * Scale;
constexpr int XMax =  1 * Scale;
constexpr int YMin = -1 * Scale;
constexpr int YMax =  1 * Scale;

constexpr int XRange = XMax - XMin;
constexpr int YRange = YMax - YMin;
constexpr int EscapeRadius = 2 * Scale;
constexpr int EscapeRadiusSquared = 4 * Scale;

using Screen = ansi::Screen<ImageWidth, ScreenHeight>;
using Palette = String<10>;

auto screen_begin = Screen::begin;
auto screen_end = Screen::end;
auto screen_clear = Screen::clear.outline();
auto screen_move_to = Screen::move_to.outline();


auto map_x = function_<s16(u8)>("map_x", "column") | define {
  auto column = var_("column").cast<s16>();

  return_(XMin + column * XRange / (ImageWidth - 1));
};


auto map_y = function_<s16(u8)>("map_y", "row") | define {
  auto row = var_("row").cast<s16>();

  return_(YMax - row * YRange / (ImageHeight - 1));
};


auto escape_iterations =
  function_<u8(s16, s16)>("escape_iterations", "cx", "cy") | define {
    auto cx = var_("cx");
    auto cy = var_("cy");

    auto zx = (let_<s16>("zx") = s16{0});
    auto zy = (let_<s16>("zy") = s16{0});

    auto zx_squared = let_<s16>("zx_squared");
    auto zy_squared = let_<s16>("zy_squared");
    auto next_zx = let_<s16>("next_zx");
    auto next_zy = let_<s16>("next_zy");

    auto iterations = (let_<u8>("iterations") = 0);

    while_(iterations < MaxIterations) {
      // Keep the next multiplication inside the signed 16-bit range.
      if_((zx > EscapeRadius) || (zx < -EscapeRadius) ||
          (zy > EscapeRadius) || (zy < -EscapeRadius)) {
        break_;
      };

      zx_squared = zx * zx / Scale;
      zy_squared = zy * zy / Scale;

      if_(zx_squared + zy_squared > EscapeRadiusSquared) {
        break_;
      };

      next_zx = zx_squared - zy_squared + cx;
      next_zy = (zx * zy / Scale) * 2 + cy;

      zx = next_zx;
      zy = next_zy;
      ++iterations;
    };

    return_(iterations);
  };


auto shade = function_<u8(u8)>("shade", "iterations") | define {
  auto iterations = var_("iterations");
  auto palette = (let_<Palette>("palette") = " .:-=+*#%@");

  return_(palette[iterations * 9 / MaxIterations]);
};


auto show_progress = function_<void(u8)>("show_progress", "row") | define {
  auto row = var_("row");

  screen_move_to(0, 0);
  print("Rendering row ");
  print(row);
  print("/");
  print(ImageHeight);
  print("   ");

  return_;
};


auto render = function_<void()>("render") | define {
  screen_clear();

  auto row = (let_<u8>("row") = 0);

  while_(row < ImageHeight) {
    show_progress(row + 1);

    // Move once per row. The pixels can then be written sequentially without
    // emitting an ANSI cursor-positioning sequence for every character.
    screen_move_to(0, row + 1);

    auto cy = (let_<s16>("cy") = map_y(row));
    auto column = (let_<u8>("column") = 0);

    while_(column < ImageWidth) {
      auto cx = (let_<s16>("cx") = map_x(column));
      auto iterations =
        (let_<u8>("iterations") = escape_iterations(cx, cy));

      put(shade(iterations));
      ++column;
    };

    ++row;
  };

  screen_move_to(0, 0);
  print("Rendering complete: ");
  print(ImageHeight);
  print("/");
  print(ImageHeight);
  print("   ");

  return_;
};


main_() {
  screen_begin();
  render();
  screen_end();

  return_;
};
```

</details>

<details>
<summary><strong>11b — Mandelbrot Set with Plain Output</strong></summary>

Source: [`11_mandelbrot_no_screen.acs`](tools/acs/examples/11_mandelbrot_no_screen.acs)
<details><summary><strong>! Spoiler Alert !</strong></summary>

```text
              .................................:-=--=@............              
            .................................::--=@=-::..............           
         .................................:::-=@@@@@@*@:...............         
       ...............................::::::--+@@@@@@@--:::::............       
      .............................:-=%*=-+@@@@@@@@@@@#@@@--:---:..........     
    ............................:::--=@@@@@@@@@@@@@@@@@@@@@#@@@@-...........    
   ..........................:::::+@+#@@@@@@@@@@@@@@@@@@@@@@@%=-::...........   
   ...............:--::::--:::::--=*@@@@@@@@@@@@@@@@@@@@@@@@@@@--#............  
  ...............::-=*@#=+@@#==--@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*=-:............ 
 ...............::-==@@@@@@@@@@@*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*............. 
 .........:::::=*==%@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*-:............. 
 ..:::----=--===@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@-:::............. 
.:.::-*---*--=@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@=-:::..............
 ........:::::-==#@*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@=-:............. 
 ..............::--#=@@@@@@@@@@@#@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@#-:............. 
  ...............::--#%%%@@@@@+==*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@#-::............ 
  ...............::-@-:::---:::--=#@@@@@@@@@@@@@@@@@@@@@@@@@@@@*=@............  
   .................:.......:::::-=%@*@@@@@@@@@@@@@@@@@@@@@@@@=-::...........   
    ............................:::--#@@@@@@@@@@@@@@@@@@@@@%@@@@:...........    
     .............................::-@@#%=*@@@@@@@@@@@@@@@--=-+-:..........     
       ..............................::::::---=@@@@@@%--::::::............      
         ................................::::-*@@@@@@%-::...............        
           ..................................::-=*@*--:..............           
              .................................:-=--=@............ 
```

</details>

```cpp
// Mandelbrot set
// Renders an 80 x 24 ASCII image using signed fixed-point arithmetic.

constexpr int Width = 80;
constexpr int Height = 24;
constexpr int MaxIterations = 24;

constexpr int Scale = 64;
constexpr int XMin = -2 * Scale;
constexpr int XMax =  1 * Scale;
constexpr int YMin = -1 * Scale;
constexpr int YMax =  1 * Scale;

constexpr int XRange = XMax - XMin;
constexpr int YRange = YMax - YMin;
constexpr int EscapeRadius = 2 * Scale;
constexpr int EscapeRadiusSquared = 4 * Scale;

using Palette = String<10>;


auto map_x = function_<s16(s16)>("map_x", "column") | define {
  auto column = var_("column");

  return_(XMin + column * XRange / (Width - 1));
};


auto map_y = function_<s16(s16)>("map_y", "row") | define {
  auto row = var_("row");

  return_(YMax - row * YRange / (Height - 1));
};


auto escape_iterations =
  function_<u8(s16, s16)>("escape_iterations", "cx", "cy") | define {
    auto cx = var_("cx");
    auto cy = var_("cy");

    auto zx = (let_<s16>("zx") = s16{0});
    auto zy = (let_<s16>("zy") = s16{0});

    auto zx_squared = let_<s16>("zx_squared");
    auto zy_squared = let_<s16>("zy_squared");
    auto next_zx = let_<s16>("next_zx");
    auto next_zy = let_<s16>("next_zy");

    auto iterations = (let_<u8>("iterations") = 0);

    while_(iterations < MaxIterations) {
      // Prevent the next multiplication from overflowing s16.
      if_((zx > EscapeRadius) || (zx < -EscapeRadius) ||
          (zy > EscapeRadius) || (zy < -EscapeRadius)) {
        break_;
      };

      zx_squared = zx * zx / Scale;
      zy_squared = zy * zy / Scale;

      if_(zx_squared + zy_squared > EscapeRadiusSquared) {
        break_;
      };

      next_zx = zx_squared - zy_squared + cx;
      next_zy = (zx * zy / Scale) * 2 + cy;

      zx = next_zx;
      zy = next_zy;
      ++iterations;
    };

    return_(iterations);
  };


auto shade = function_<u8(u8)>("shade", "iterations") | define {
  auto iterations = var_("iterations");
  auto palette = (let_<Palette>("palette") = " .:-=+*#%@");

  return_(palette[iterations * 9 / MaxIterations]);
};


auto render = function_<void()>("render") | define {
  auto row = (let_<s16>("row") = s16{0});

  while_(row < Height) {
    auto cy = (let_<s16>("cy") = map_y(row));
    auto column = (let_<s16>("column") = s16{0});

    while_(column < Width) {
      auto cx = (let_<s16>("cx") = map_x(column));
      auto iterations =
        (let_<u8>("iterations") = escape_iterations(cx, cy));

      put(shade(iterations));
      ++column;
    };

    println();
    ++row;
  };

  return_;
};


main_() {
  render();
  return_;
};
```

</details>

<details>
<summary><strong>12 — Pong</strong></summary>

Source: [`12_pong.acs`](tools/acs/examples/12_pong.acs)
Note: this should be run in the gaming-mode of bfint!

```cpp

constexpr size_t ScreenWidth  = 40;
constexpr size_t ScreenHeight = 20;
constexpr size_t PaddleHeight = 4;

constexpr size_t LeftPaddleX  = 2;
constexpr size_t RightPaddleX = ScreenWidth - 3;

constexpr size_t MinPaddleY = 1;
constexpr size_t MaxPaddleY = ScreenHeight - PaddleHeight - 1;

constexpr size_t InitialPaddleY = ScreenHeight / 2 - PaddleHeight / 2;
constexpr size_t InitialBallX   = ScreenWidth / 2;
constexpr size_t InitialBallY   = ScreenHeight / 2;

constexpr size_t BallDelay = 10;

using Screen = ansi::Screen<ScreenWidth, ScreenHeight>;


global_<u8>("leftY");
global_<u8>("rightY");

global_<u8>("ballX");
global_<u8>("ballY");
global_<u8>("ballRight");
global_<u8>("ballDown");

global_<u8>("running");
global_<u8>("tick");


auto drawPaddle =
  function_<void(u8, u8, u8)>("drawPaddle", "x", "y", "glyph") | define {
  auto x     = var_("x");
  auto y     = var_("y");
  auto glyph = var_("glyph");

  Screen::put(x, y, glyph);
  Screen::put(x, y + 1, glyph);
  Screen::put(x, y + 2, glyph);
  Screen::put(x, y + 3, glyph);

  return_;
};


auto drawBorder =
  function_<void()>("drawBorder") | define {
  for (size_t x = 0; x < ScreenWidth; ++x) {
    Screen::put(x, 0, '-');
    Screen::put(x, ScreenHeight - 1, '-');
  }

  for (size_t y = 1; y < ScreenHeight - 1; ++y) {
    Screen::put(0, y, '|');
    Screen::put(ScreenWidth - 1, y, '|');
  }

  Screen::put(0, 0, '+');
  Screen::put(ScreenWidth - 1, 0, '+');
  Screen::put(0, ScreenHeight - 1, '+');
  Screen::put(ScreenWidth - 1, ScreenHeight - 1, '+');

  return_;
};


auto resetBall =
  function_<void(u8, u8)>("resetBall", "right", "down") | define {
  auto ballX     = var_("ballX");
  auto ballY     = var_("ballY");
  auto ballRight = var_("ballRight");
  auto ballDown  = var_("ballDown");

  ballX     = InitialBallX;
  ballY     = InitialBallY;
  ballRight = var_("right");
  ballDown  = var_("down");

  return_;
};


auto handleInput =
  function_<void(u8)>("handleInput", "key") | define {
  auto key     = var_("key");
  auto leftY   = var_("leftY");
  auto rightY  = var_("rightY");
  auto running = var_("running");

  if_(key == 'q') {
    running = 0;
  };

  if_(key == 'w') {
    if_(leftY > MinPaddleY) {
      drawPaddle(LeftPaddleX, leftY, ' ');
      --leftY;
      drawPaddle(LeftPaddleX, leftY, '#');
    };
  };

  if_(key == 's') {
    if_(leftY < MaxPaddleY) {
      drawPaddle(LeftPaddleX, leftY, ' ');
      ++leftY;
      drawPaddle(LeftPaddleX, leftY, '#');
    };
  };

  if_(key == 'o') {
    if_(rightY > MinPaddleY) {
      drawPaddle(RightPaddleX, rightY, ' ');
      --rightY;
      drawPaddle(RightPaddleX, rightY, '#');
    };
  };

  if_(key == 'l') {
    if_(rightY < MaxPaddleY) {
      drawPaddle(RightPaddleX, rightY, ' ');
      ++rightY;
      drawPaddle(RightPaddleX, rightY, '#');
    };
  };

  return_;
};


auto stepBall =
  function_<void()>("stepBall") | define {
  auto leftY    = var_("leftY");
  auto rightY   = var_("rightY");
  auto ballX    = var_("ballX");
  auto ballY    = var_("ballY");
  auto movingRight = var_("ballRight");
  auto movingDown  = var_("ballDown");

  Screen::put(ballX, ballY, ' ');

  /*
   * Vertical movement
   */
  if_(movingDown) {
    if_(ballY == ScreenHeight - 2) {
      movingDown = 0;
      --ballY;
    }
    else_ {
      ++ballY;
    };
  }
  else_ {
    if_(ballY == 1) {
      movingDown = 1;
      ++ballY;
    }
    else_ {
      --ballY;
    };
  };

  /*
   * Horizontal movement and paddle collisions
   */
  if_(movingRight) {
    if_(ballX == RightPaddleX - 1) {
      if_(ballY >= rightY && ballY < rightY + PaddleHeight) {
	movingRight = 0;
	--ballX;
      }
      else_ {
	resetBall(0, 1);
      };
    }
    else_ {
      ++ballX;
    };
  }
  else_ {
    if_(ballX == LeftPaddleX + 1) {
      if_(ballY >= leftY && ballY < leftY + PaddleHeight) {
	movingRight = 1;
	++ballX;
      }
      else_ {
	resetBall(1, 0);
      };
    }
    else_ {
      --ballX;
    };
  };

  Screen::put(ballX, ballY, 'O');
  return_;
};


main_() {
  auto leftY  = var_("leftY");
  auto rightY = var_("rightY");
  auto running = var_("running");
  auto tick    = var_("tick");

  leftY  = InitialPaddleY;
  rightY = InitialPaddleY;
  running = 1;
  tick    = 0;

  resetBall(1, 1);

  Screen::clear();
  drawBorder();

  drawPaddle(LeftPaddleX, leftY, '#');
  drawPaddle(RightPaddleX, rightY, '#');
  Screen::put(var_("ballX"), var_("ballY"), 'O');

  auto key = let_<u8>("key");

  while_(running) {
    read(key);
    handleInput(key);

    ++tick;

    if_(tick == BallDelay) {
      tick = 0;
      stepBall();
    };
  };

  Screen::clear();
  return_;
};
```

</details>



## License

Acus and Acus Sugar are licensed under the GNU General Public License v3.0 or later. See `LICENSE` for details.
