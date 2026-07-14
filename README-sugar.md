<p align="center">
  <img src="../../../assets/logo_transparent.png" alt="Acus logo" width="180">
</p>

# Acus Sugar

Acus Sugar is a C++23 embedded language for generating structured Brainfuck programs. It sits on top of the Acus backend and provides familiar syntax for variables, expressions, functions, loops, conditionals, arrays, structs, pointers, globals, recursion, and basic I/O.

Instead of writing Brainfuck directly:

```brainfuck
++++++++[>++++++++<-]>+.+.+.
```

an Acus Sugar program can look like this:

```cpp
function_<void()>("main") | define {
  println("Hello, World!");
  return_;
};
```

Acus lowers the resulting program to Brainfuck source code.

> [!NOTE]
> Acus Sugar is under active development. The syntax and standard-library API may still change.

## Why Acus Sugar?

The lower-level Acus API exposes the compiler backend directly. That is useful for building frontends and experimenting with code generation, but it is intentionally explicit. Acus Sugar adds a more language-like layer while retaining the advantages of C++:

- compile-time type checking;
- templates and reusable C++ abstractions;
- source-location-aware diagnostics;
- direct access to the lower-level Acus API when needed;
- no parser or separate language toolchain required.

For users who do want standalone source files, the optional `acs` frontend wraps `.acs` files in the necessary C++ boilerplate, compiles them, and emits Brainfuck.

## Requirements

- A C++23 compiler
- GNU Make
- The Acus static library and public headers

Build and install Acus from the repository root:

```sh
make
sudo make install
```

A user-local installation can be made with:

```sh
make install PREFIX="$HOME/.local"
```

## Minimal C++ example

```cpp
#include <iostream>
#include <acus/sugar/sugar.h>
#include <acus/sugar/std.h>

using namespace acus::sugar;
using namespace acus::sugar::io;

int main() try {
  program("hello");
  {
    function_<void()>("main") | define {
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

Compile and link it with:

```sh
c++ -std=c++23 hello.cc -lacus -o hello
./hello > hello.bf
```

For a non-standard installation prefix:

```sh
c++ -std=c++23 hello.cc \
  -I"$HOME/.local/include" \
  -L"$HOME/.local/lib" \
  -lacus -o hello
```

## Using the `acs` frontend

The `acs` frontend lets a file contain only the Acus program definition. A file named `hello.acs` can contain:

```cpp
function_<void()>("main") | define {
  println("Hello, World!");
  return_;
};
```

Compile it to Brainfuck with:

```sh
acs hello.acs -o hello.bf
```

The frontend supplies a standard program name, uses `main` as the entry function, compiles the generated C++ wrapper, and runs it to produce Brainfuck source.

Useful options include:

```sh
acs hello.acs --compiler clang++
acs hello.acs --acus-root /path/to/acus
acs hello.acs --acus-prefix "$HOME/.local"
acs hello.acs -I custom/include -L custom/lib
acs hello.acs --emit-cpp generated.cc
```

A statement-only file can be compiled with `--main-body`:

```cpp
println("Hello from main");
```

```sh
acs hello.acs --main-body -o hello.bf
```

The wrapper adds the enclosing `main` function and its final `return_;`.

## The two levels of execution

Acus Sugar is embedded in C++, so it is important to distinguish two kinds of control flow.

Ordinary C++ executes while the Brainfuck program is being generated:

```cpp
for (int i = 0; i < 3; ++i) {
  println("This statement is emitted three times");
}
```

Acus Sugar control flow becomes runtime logic in the generated Brainfuck program:

```cpp
for_(let<u8>("i") = 0, var("i") < 3, ++var("i")) {
  println(var("i"));
};
```

The same distinction applies to `if` versus `if_`, and `while` versus `while_`.

## Program structure

A program is opened with `program()` and finalized with `endProgram()`:

```cpp
program("example", "main");
{
  // Globals and functions
}
endProgram();
```

The second argument is the entry-function name and defaults to `"main"`.

After finalization, retrieve the generated source with:

```cpp
auto bf = generateBrainfuck("example");
```

## Types

### Integers

Acus Sugar currently provides four fixed-width integer types:

| Type | Description |
|---|---|
| `u8` | Unsigned 8-bit integer |
| `s8` | Signed 8-bit integer |
| `u16` | Unsigned 16-bit integer |
| `s16` | Signed 16-bit integer |

Examples:

```cpp
let<u8>("small") = 42;
let<s16>("temperature") = -1200;
```

Integer arithmetic follows the fixed-width representation of the selected type. Overflow wraps according to the backend's integer semantics.

### Strings

`string<N>` stores a string with a fixed maximum capacity:

```cpp
let<string<20>>("name") = "Ada";
println(var("name"));
```

### Arrays

```cpp
using Scores = Array<u8, 4>;

let<Scores>("scores") = Scores{10, 20, 30, 40};
println(var("scores")[2]);

let<u8>("i") = 1;
var("scores")[var("i")] = 99;
```

Arrays support both constant and runtime indexing.

### Structs

Structs are declared as C++ types:

```cpp
using Point = Struct<"Point",
  Field<"x", u8>,
  Field<"y", u8>
>;
```

They can then be declared and accessed as Acus values:

```cpp
let<Point>("p") = Point{10, 20};
var("p").field("x") = 42;
println(var("p").field("y"));
```

### Pointers

```cpp
let<u8>("value") = 10;
let<ptr<u8>>("p") = &var("value");

*var("p") = 20;
println(var("value"));
```

Pointer arithmetic and pointers to aggregate elements are supported where the underlying type rules allow them.

## Variables and expressions

Declare a local variable with `let<T>()`:

```cpp
let<u8>("x") = 5;
```

Refer to an existing variable with `var()`:

```cpp
var("x") += 3;
println(var("x"));
```

`let()` and `var()` return `Expr` objects. Expressions support:

```cpp
+  -  *  /  %
<  <=  >  >=  ==  !=
&&  ||  !
+=  -=  *=  /=  %=
++  --
```

Unary address-of and dereference use the normal C++ operators:

```cpp
&var("x")
*var("p")
```

Cast an expression with:

```cpp
let<u16>("wide") = var("x").cast<u16>();
```

Acus expressions describe generated runtime operations. In particular, do not assume that logical operators inherit C++ short-circuit behaviour.

## Globals

Globals are declared at program scope:

```cpp
global<string<20>>("message");

function_<void()>("main") | define {
  var("message") = "Hello";
  println(var("message"));
  return_;
};
```

## Control flow

### Conditionals

```cpp
if_(var("x") < 10) {
  println("small");
} else_ {
  println("large");
};
```

An `else_` block is optional:

```cpp
if_(var("x") == 0) {
  println("zero");
};
```

### While loops

```cpp
while_(var("x") > 0) {
  println(var("x"));
  --var("x");
};
```

### For loops

```cpp
for_(let<u8>("i") = 0,
     var("i") < 10,
     ++var("i")) {
  println(var("i"));
};
```

### Break and continue

```cpp
for_(let<u8>("i") = 0, var("i") < 20, ++var("i")) {
  if_(var("i") == 10) {
    break_;
  };

  if_(var("i") % 2 == 0) {
    continue_;
  };

  println(var("i"));
};
```

## Functions

Function signatures use ordinary C++ function-type syntax.

### Defining a function

```cpp
auto add = function_<u16(u16, u16)>("add", "a", "b") | define {
  return_(var("a") + var("b"));
};
```

Call the returned typed handle like a normal function object:

```cpp
println(add(10, 20));
```

### Forward declarations

Forward declarations are useful for recursion and mutually dependent functions:

```cpp
auto fib = function_<u16(u16)>("fib") | declare;

function_<u16(u16)>("fib", "n") | define {
  if_(var("n") < 2) {
    return_(var("n"));
  } else_ {
    return_(fib(var("n") - 1) + fib(var("n") - 2));
  };
};
```

### Void returns

Use `return_;` for a void return:

```cpp
function_<void()>("hello") | define {
  println("Hello");
  return_;
};
```

Use `return_(expression);` when returning a value.

## Input and output

The basic I/O functions live in `acus::sugar::io`:

```cpp
print('A');
print(123);
print(var("value"));

println("Hello");
println(var("value"));
println();

read(var("character"));
```

`print()` prints integers in decimal form and writes printable aggregate values according to the underlying Acus type. `read()` reads one Brainfuck input byte into its destination.

## Standard-library functions

The standard-library layer currently includes helpers such as:

```cpp
io::readLine<20>()
io::strlen<u8>(text)
io::stringToInt<s16>(text)
```

These helpers can generally be used in two ways.

### Inline form

```cpp
auto line = io::readLine<20>();
println(io::strlen<u8>(line));
```

The implementation is emitted directly at the call site. This avoids function-call and argument-copying overhead, but repeated use can increase generated program size.

### Outlined form

```cpp
auto readLine = io::readLine<20>.outline();
auto strlen20 = io::strlen<u8, 20>.outline();

function_<void()>("main") | define {
  auto line = readLine();
  println(strlen20(line));
  return_;
};
```

Outlining emits one reusable Acus function and returns a typed handle to it. This can reduce generated code size when a helper is called repeatedly, at the cost of function-call and data-copying overhead.

The exact standard-library surface is still evolving.

## A larger example

```cpp
#include <iostream>
#include <acus/sugar/sugar.h>
#include <acus/sugar/std.h>

using namespace acus::sugar;
using namespace acus::sugar::io;

int main() {
  program("fibonacci");
  {
    function_<void()>("main") | define {
      let<u8>("count") = 10;
      let<u16>("a") = 0;
      let<u16>("b") = 1;

      for_(let<u8>("i") = 0,
           var("i") < var("count"),
           ++var("i")) {
        println(var("a"));

        let<u16>("next") = var("a") + var("b");
        var("a") = var("b");
        var("b") = var("next");
      };

      return_;
    };
  }
  endProgram();

  std::cout << generateBrainfuck("fibonacci");
}
```

## Interoperating with the low-level API

The sugar layer is built directly on the Acus `Assembler`. It is therefore possible to drop down to the lower-level API when a sugar abstraction is not yet available.

The current global assembler is exposed as:

```cpp
acus::sugar::__assembler
```

This is primarily intended for library implementation and advanced experimentation. Code using it directly is more closely coupled to Acus internals than ordinary sugar code.

## Diagnostics

The sugar API records `std::source_location` information for most operations. Errors therefore point to the C++ or `.acs` source location that requested the invalid operation rather than only to an internal compiler function.

The `acs` frontend additionally inserts `#line` directives so compiler diagnostics refer to the original `.acs` file.

## Current limitations

Acus Sugar is not yet a general-purpose production language. Current constraints include:

- only 8- and 16-bit integer types;
- fixed-capacity strings and arrays;
- generated Brainfuck can become large and slow;
- some operations involve explicit copying because Brainfuck has no native call stack or references;
- the standard library is still small;
- the C++ API currently uses a shared assembler and is not designed for concurrent program generation;
- API stability is not yet guaranteed.

These limitations are part of the project’s purpose: Acus explores how much structured language machinery can be compiled into plain Brainfuck.

## Examples

The repository includes focused examples covering:

- Hello World;
- variables and arithmetic;
- control flow;
- functions and recursion;
- arrays and structs;
- pointers;
- globals;
- strings and input;
- standard-library outlining;
- `.acs` statement-only mode.

Build the examples with:

```sh
make examples
```

## Contributing

Bug reports, tests, documentation improvements, and small standard-library additions are welcome. Because the API is still evolving, larger design changes are best discussed before implementation.

In particular, useful contributions include:

- minimal failing programs;
- comparisons of generated Brainfuck size or runtime;
- new examples;
- reusable algorithms that fit the inline/outlined standard-library model;
- experimental frontends built on Acus.

## License

Acus and Acus Sugar are licensed under the GNU General Public License v3.0 or later. See `LICENSE` for details.
