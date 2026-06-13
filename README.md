<p align="center"><img src="assets/logo_transparent.png" alt="Acus logo" width="200"/></p>

# Acus

Acus is a C++23 backend toolkit for generating Brainfuck programs through an API that resembles a simple programming language. It lets you construct a program in C++ using functions, scopes, local and global variables, arrays, structs, pointers, function pointers, arithmetic, comparisons, logical operations, labels, branches, and returns. Acus then lowers that structured program to Brainfuck.

The intended use case is to build higher-level frontends on top of Acus while still keeping the backend usable directly from C++ for experiments, tests, and low-level code generation.

#### Let's be real
Above all, this library is meant to have fun with. Brainfuck is a language that exists for esoteric reasons, not practical use. If you create some weird language on top of Acus to generate BF, let me know and I will link to it from this page! Contact me at [this_github_username]@gmail.com.

## Minimal example: Hello, World!

The program below features a simple Hello World example and outlines the way an Acus program must be structured:

```cpp
#include <iostream>
#include <acus/acus.h>

using namespace acus::api;

int main() try {
  Assembler a;

  a.program("hello", "main").begin(); {

    a.function("main").begin(); {
      a.print(literal::string("Hello, World!\n"));
      a.returnFromFunction();
    } a.endFunction();

  } a.endProgram();

  std::cout << a.brainfuck("hello") << '\n';
}
catch (Error const &e) {
  std::cerr << e.what() << " (" << e.code() << ")\n";
}
```

This constructs a program named `hello`, defines `main` as its entry function that simply prints  `Hello, World!`. After defining the BF-program, the generated Brainfuck source is printed to `stdout`. More involved examples are included below.

## Building
To start off, clone this repository:
```sh
git clone https://github.com/jorenheit/acus
```
Acus uses a small Make-based build system. The project has no external dependencies beyond a C++23 compiler, `make`, and the C++ standard library.

### Build targets

The root Makefile provides the following targets:

| Target              | Description                                     |
|---------------------|-------------------------------------------------|
| `make` / `make all` | Build the static Acus library                   |
| `make examples`     | Build all examples in the `examples/` directory |
| `make tests`        | Build the test binaries                         |
| `make check`        | Build and run the test suite                    |
| `make install`      | Install the static library and public headers   |
| `make uninstall`    | Remove installed Acus files                     |
| `make clean`        | Remove generated build artifacts                |

### Building the static library

To build Acus as a static library:

```sh
make
```

The resulting archive is written to:

```text
lib/libacus.a
```

### Installing

To install the static library and public headers system-wide:

```sh
sudo make install
```

By default, this installs into `/usr/local`:

```text
/usr/local/lib/libacus.a
/usr/local/include/acus/
```

To install somewhere else, override `PREFIX`:

```sh
make install PREFIX="$HOME/.local"
```

This installs to:

```text
$HOME/.local/lib/libacus.a
$HOME/.local/include/acus/
```

When using a non-standard install prefix, you may need to pass the include and
library paths explicitly when compiling programs that use Acus:

```sh
c++ -std=c++23 hello.cc -I$HOME/.local/include -L$HOME/.local/lib -lacus
```

### Building examples

Examples live in the `examples/` directory. They can be built from the root directory with:

```sh
make examples
```

This delegates to the Makefile inside `examples/`. You can also build the examples directly:

```sh
cd examples
make
```

The examples Makefile assumes that the Acus static library has already been built and is available at:

```text
../lib/libacus.a
```

Example binaries are written to:

```text
examples/bin/
```

### Running tests

To build the tests:

```sh
make tests
```

To build and run the tests:

```sh
make check
```

### Uninstalling

If Acus was installed with `make install`, it can be removed with:

```sh
sudo make uninstall
```

or, if a custom prefix was used:

```sh
make uninstall PREFIX="$HOME/.local"
```

### Using Acus from another project

After installation, a program can be linked against Acus like this:

```sh
c++ -std=c++23 main.cc -lacus
```

If Acus has not been installed system-wide, point the compiler at the local
headers and static library:

```sh
c++ -std=c++23 main.cc -I/path/to/acus/include -L/path/to/acus/lib -lacus
```

For example, from a project next to the Acus source tree:

```sh
c++ -std=c++23 main.cc -I../acus/include -L../acus/lib -lacus
```

## Public include files

To use the library, you need to include the main Acus header: `acus.h`. This header will include all necessary headers for you and expose the `acus::api` namespace that you can import for ease of use.

```cpp
#include <acus/acus.h>
using namespace acus::api;
```
The table below shows the classes and namespaces available after `acus.h` has been included (full path) and, optionally, when the `acus::api` namespace is imported (short path):

| Full path                | Short path         | type       | Description                                         |
|--------------------------|--------------------|------------|-----------------------------------------------------|
| `acus::Assembler`        | `Assembler`        | class      | The main assembler class used to generate BF.       |
| `acus::literal`          | `literal`          | namespace  | Namespace for literal-generation.                   |
| `acus::literal::Literal` | `Literal`          | class      | Literal object, representing a compile-time value.  |
| `acus::ts`               | `ts`               | namespace  | Typesystem namespace, used to request type-handles. |
| `acus::ts::TypeHandle`   | `TypeHandle`       | class      | Object representing a type.                         |
| `acus::error::Error`     | `error::Error`     | class      | Error object, thrown when an error occurs.          |
| `acus::error::ErrorCode` | `error::ErrorCode` | enum class | Error code embedded in the Error object.            |

## Acus Programming
### `Assembler`

`acus::Assembler` is the main public entry point. It owns the program under construction and exposes methods for declaring data, defining functions, creating expressions, emitting control flow, and generating Brainfuck.

A typical program has this shape:

```cpp
Assembler a;

a.program("program_name", "main").begin(); {
  // global variable declarations

  a.function("main").begin(); {
    // Function body
    a.returnFromFunction();
  } a.endFunction();
  
  // Other functions
  
} a.endProgram();

std::string bf = c.brainfuck("program_name");
```

The usage of C++ scope blocks is mainly for convenience (the C++ scope matches the BF scope and indentation is handled automatically by your editor) but not necessary for creating valid programs.

### Functions
#### Defining a Function

A function is defined using `Assembler::function`, which returns a `FunctionBuilder` object. You then call the `param` and `ret` methods on this object until the entire function signature has been completed, at which point you call `begin` to start the body of the function:

```cpp
a.function("foo")
  .param("x", ts::u8())
  .ret(ts::u8())
  .begin();
```
This starts a function that takes a single `u8` parameter named `x` and returns an `u8`. A simpler function that takes no arguments and returns void might simply be defined with: 
```cpp
a.function("bar").begin();
```
The function-body is closed by calling `endFunction()`. Before closing a function, all flow-paths within that function should either end in a return (`Assembler::returnFromFunction(...)`) or an abort (`Assembler::abortProgram`). If there exists a flow-path that does not terminate in a return, an error will be thrown. If a function was defined with a return-type, a value of a compatible type must be passed to `returnFromFunction`. `abortProgram` is called without arguments and will terminate the entire program immediately without return-values being evaluated.

```cpp
a.function("universe")
  .ret(ts::u8())
  .begin();
{
  a.returnFromFunction(literal::u8(42));
} a.endFunction();

```

#### Calling a Function

Function calls use the same builder-pattern as function definitions, the difference being that you call `arg` to pass arguments (one by one), `into` to specify the target to store the return-value into and `done` to finish the builder and perform the call. 

```cpp
a.callFunction("foo")
  .arg("x")
  .into("y")
  .done();
```
In the snippet above, there has to exist a variable `x` with a type that is compatible with the first (and only) parameter of `foo`, and another variable `y` that is compatible with the return-type of `foo`. Note that, while many API calls return `Expression` objects, a function-call returns void and cannot therefore be embedded in other API calls that expect `Expression`s.

### Expressions

Most expression-operations accept flexible operands such as:

- variable names, e.g. `"x"`,
- literals, e.g. `literal::u8(42)`,
- expressions returned by other assembler calls,
- array elements,
- struct fields,
- dereferenced pointers.

For example:

```cpp
a.assign("x", literal::u8(10));             // x = 10
a.assign("y", c.add("x", literal::u8(5)));  // y = x + 5
a.print(c.arrayElement("arr", "idx"));      // print(arr[idx]) 
```

### Data types

Types are created through the `acus::ts` namespace. Each type-factory returns a `ts::TypeHandle` object (just a pointer internally) that you can pass to other API functions that require a type.

#### Integer types
There are four integer types available: two unsigned types and two signed types, both available in 8 or 16 bits:

```cpp
TypeHandle u8  = ts::u8();
TypeHandle s8  = ts::s8();
TypeHandle u16 = ts::u16();
TypeHandle s16 = ts::s16();
```

#### Arrays and strings
Array-types can be defined by passing a previously defined type and the size of the required array to `ts::array`. This will return a unique handle for each unique type/size combination. Strings are more flexible. They are defined by their maximum capacity but can be assigned to other strings of larger capacity; this does not hold for arrays.

```cpp
auto bytes4 = ts::array(u8, 4);
auto string  = ts::string(32);
```

Arrays and strings can be indexed using `Assembler::arrayElement`:

```cpp
a.declareLocal("arr", string);
a.declareLocal("idx", u8);
c.assign("str", literal::string("Hello"));

Literal u8_1 = literal::u8(1);
c.assign("idx", u8_1);
c.write(c.arrayElement("str", "idx")); // runtime index (slow) -> prints 'e'
c.write(c.arrayElement("str", u8_1));  // literal index (fast) -> prints 'e'
```

In the example above, `idx` is a runtime variable (even though we can see what its value will be), so finding the appropriate address in memory will be a lot slower compared to using a literal index.

#### Structs

Struct types are built with `ts::defineStruct(name)`. This function returns a builder object that expects you to call `field` for each field of the struct and finish the definition by calling `done`:

```cpp
auto point = ts::defineStruct("Point")
  .field("x", u8)
  .field("y", u8)
  .done();
```

Struct fields can be accessed by either their name or by index:

```cpp
a.declareLocal("p", point);
auto x = a.structField("p", "x"); // p.x
auto y = a.structField("p", 1);   // p.y
```

#### Pointers
Acus supports pointers (and function pointers, see below):

```cpp
auto u8ptr = ts::pointer(u8); 
```

Pointers can point to locals, globals, array elements, struct fields, and other addressable objects and are initialized using the `Assembler::addressOf` function:

```cpp
a.declareLocal("p", u8ptr);
a.declareLocal("x", u8);

a.assign("p", a.addressOf("x"));
a.assign(a.dereferencePointer("p"), literal::u8('A'));
a.write("x"); // writes 'A' to the output
```

#### Function types and function pointers

To create a function-pointer type, one must first define the type of the function that it points to, with `ts::function()`. This is similar to a function definition, except that no parameter-names are passed to the `param` function.

```cpp
// fnType takes u8 and returns u8
auto fnType = ts::function()
  .ret(u8)
  .param(u8)
  .done();
```

Function pointer types can then be created from function types:

```cpp
auto fPtr = ts::function_pointer(fnType);
```

Function pointers are initialized through literals that refer to named functions and called similarly to normal functions, using `Assembler::callFunctionPointer` instead (the builder works exactly the same):

```cpp
a.declareLocal("fptr", ts::function_pointer(fnType));
a.assign("fptr", literal::function_pointer(fnType, "foo"));
a.callFunctionPointer("fptr").arg("x").into("y").done();
```

### Literals
Literals (represented by the `Literal` type) are compile-time objects that can be used to initialize runtime objects.


#### Integers and Strings
Literals are created through the `acus::literal` namespace. Each builtin type has a corresponding literal factory:

```cpp
literal::u8(65)
literal::s8(-3)
literal::u16(1024)
literal::s16(-300)
literal::string("Hello")
```

#### Arrays
Arrays use builders to compose multiple literals into an array-literal:

```cpp
auto arrType = ts::array(ts::u8(), 4);

auto arr = literal::array(arrType)
  .push(literal::u8('A'))
  .push(literal::u8('B'))
  .push(literal::u8('C'))
  .push(literal::u8('D'))
  .done();
```

#### Struct
One can also build literal struct instantiations at compile-time, once a struct type has been defined:

```cpp
auto point = ts::defineStruct("Point")
  .field("x", ts::u8())
  .field("y", ts::u8())
  .done();

auto p = literal::struct_t(point)
  .init("x", literal::u8('A'))
  .init("y", literal::u8('B'))
  .done();
```

### Global Variables

Global variables are declared inside a program, before the first function-definition. Every function will then be able to access this variable through its name:

```cpp
a.program("globals", "main").begin(); {

  a.declareGlobal("g", ts::u8());

  a.function("main").begin(); {
    a.assign("g", literal::u8('A'));
    a.callFunction("foo").done();
    a.write("g"); // writes F
    a.returnFromFunction();
  } a.endFunction();

  a.function("foo").begin(); {
    a.assign("g", literal::u8('F'));
    a.returnFromFunction();
  } a.endFunction();

} a.endProgram();
```

### Input/Output
#### Input
To read a single byte from the input, `Assembler::read` is called, where the argument refers to the destination variable. This destination may refer to any variable of size 1, but most commonly this is `u8`:
```cpp
a.declareLocal("x", u8);
a.read("x");
```

Note that, depending on terminal settings, your terminal will often buffer until you press Enter (which is a character by itself). Acus does not implement any input-parsing. Many terminals support Ctrl+D for end of input.

#### Output
Acus has two main output operations:

```cpp
a.write(value);
a.print(value);
```

`write` writes the raw byte or bytes represented by the value. Arrays and structs can are written word-wise, where 16-bit fields are written in big endian order.

`print` is defined for integers and strings. Integers are sent to the output in decimal notation; it supports the integer widths and signedness currently provided by the type system, for example `u8`, `s8`, `u16`, and `s16`. Strings are written until the terminating null is encountered (in contrast, when passing a string to `write`, all bytes are written, even if the string only takes up part of the total capacity).

```cpp
a.declareLocal("x", ts::s16());
a.assign("x", literal::s16(-300));
a.print("x"); // "-300"
```

### Control flow

Control-flow within a function is achieved by setting labels and jumping between those labels. This is all that's needed to create conditionals and loops. A label is set using `Assembler::label`, which can be jumped to unconditionally using `Assembler::jump` or conditionally using `Assembler::jumpIf`. The latter takes an expression that will be evaluated as a boolean expression (any nonzero will evaluate to true), the label to jump to when the expression evaluates to true, and the label to jump to when the expression evaluates to false. In the example below, a for-loop that would be written in C/C++ as `for (int i = 0; i < n; ++i) { ... }`, is implemented using labels and jumps:

```cpp
a.assign("i", literal::u8(0));
a.label("check");
a.jumpIf(c.lt("i", "n"), "body", "done");
a.label("body");
// ...
a.addAssign("i", literal::u8(1));
a.jump("check");
a.label("done");
```
### Scope

Even though functions already have their own local scope, it can be useful to subdivide that scope into smaller scopes. Variables declared within scopes will be freed at the end of their scope, so this is a way to avoid nameclashes and to manage stack-memory in a simple way. A scope is started using a builder for consistency's sake, but it does not provide any modifying functions. Simply:

```cpp
a.scope().begin(); {
// ...
} a.endScope();

```

### Integer Casts
Integer types may be implicitly cast when there is no size or signedness conflict. For example, a `u8` may be used wherever a `u16` is expected. For all other conversions, an explicit cast can be used with `Assembler::cast`.

```cpp
a.declareLocal("x", s8);
a.declareLocal("y", u8);

a.assign("y", literal::u8(200)); 
a.assign("x", "y");             // fails, different signedness
a.assign("x", a.cast("y", s8)); // OK
```

### Unary and Binary Operations
Many common unary and binary operations have been defined for all integer types. These have dedicated functions (see the full API table below) but can also be called using generic `unOp/unOpAssign` and `binOp/binOpAssign` functions, that take an enum-value as their first argument.

```cpp
a.negate("x");
a.unOp(UnOp::Neg, "x");

a.add("x", "y");
a.binOp(BinOp::Add, "x", "y"); 
```

#### Unary Operators
The table below lists all unary operations. Each of these has a `xxxAssign` version, where the operator is applied to the operand immediately.

| Function                | `UnOp::xxx` |
|-------------------------|-------------|
| `lnot/lnotAssign`       | `Not`       |
| `lbool/lboolAssign`     | `Bool`      |
| `negate/negateAssign`   | `Neg`       |
| `abs/absAssign`         | `Abs`       |
| `signBit/signBitAssign` | `SignBit`   |

#### Binary Operators
The table below lists all binary operations. Each of these has a `xxxAssign` version, where the operator is applied to the left-hand operand immediately.

| Function            | `BinOp::xxx` |
|---------------------|--------------|
| `add/addAssign`     | `Add`        |
| `sub/subAssign`     | `Sub`        |
| `mul/mulAssign`     | `Mul`        |
| `div/divAssign`     | `Div`        |
| `mod/modAssign`     | `Mod`        |
| `lnand/lnandAssign` | `Nand`       |
| `land/landAssign`   | `And`        |
| `lor/lorAssign`     | `Or`         |
| `lnor/lnorAssign`   | `Nor`        |
| `lxor/lxorAssign`   | `Xor`        |
| `lxnor/lxnorAssign` | `Xnor`       |
| `eq/eqAssign`       | `Eq`         |
| `neq/neqAssign`     | `Neq`        |
| `lt/ltAssign`       | `Lt`         |
| `le/leAssign`       | `Le`         |
| `gt/gtAssign`       | `Gt`         |
| `ge/geAssign`       | `Ge`         |


## Public API overview
A full overview of all API functions is shown in the table below:

### `acus::Assembler`

#### Construction and output

| Function     | Returns       | Description                                                                                 |
|--------------|---------------|---------------------------------------------------------------------------------------------|
| `primitives` | `std::string` | Returns a textual dump of the generated primitive instruction sequence for a named program. |
| `brainfuck`  | `std::string` | Returns the generated Brainfuck source for a named program.                                 |

#### Program, function, and scope structure

| Function      | Returns           | Description                                                                                                   |
|---------------|-------------------|---------------------------------------------------------------------------------------------------------------|
| `program`     | `ProgramBuilder`  | Creates a program builder for a named program and entry function. Finalize with `begin`.                      |
| `function`    | `FunctionBuilder` | Creates a function builder. Add parameters/return type if needed, then finalize with `begin`.                 |
| `scope`       | `ScopeBuilder`    | Creates a nested lexical scope builder. Finalize with `begin`.                                                |
| `endProgram`  | `void`            | Finishes the current program, validates it, constructs generated support blocks, and stores generated output. |
| `endFunction` | `void`            | Ends the current function.                                                                                    |
| `endScope`    | `void`            | Ends the current lexical scope and frees its locals.                                                          |

#### Declarations and global visibility

| Function        | Returns      | Description                                                                                      |
|-----------------|--------------|--------------------------------------------------------------------------------------------------|
| `declareLocal`  | `Expression` | Declares a local variable in the current scope.                                                  |
| `declareGlobal` | `void`       | Declares a global variable in the current program.                                               |
| `referGlobals`  | `void`       | Makes selected globals visible inside the current function through local global-reference slots. |

#### Function calls and returns

| Function              | Returns               | Description                                                        |
|-----------------------|-----------------------|--------------------------------------------------------------------|
| `callFunction`        | `FunctionCallBuilder` | Starts a direct function call.                                     |
| `callFunctionPointer` | `FunctionCallBuilder` | Starts an indirect function-pointer call.                          |
| `returnFromFunction`  | `void`                | Returns from the current function, optionally with a return value. |
| `abortProgram`        | `void`                | Emits an abort path for the program.                               |

#### Expression conversion, assignment, and casting

| Function | Returns      | Description                                                                        |
|----------|--------------|------------------------------------------------------------------------------------|
| `assign` | `Expression` | Assigns a value into an lvalue expression and returns the assigned lvalue.         |
| `cast`   | `Expression` | Converts an (integer) expression to another type and returns the converted result. |

#### Access expressions

| Function             | Returns      | Description                                                                                |
|----------------------|--------------|--------------------------------------------------------------------------------------------|
| `structField`        | `Expression` | Produces an expression for a struct field, selected by field name or field index.          |
| `arrayElement`       | `Expression` | Produces an expression for an array/string element, selected by constant or runtime index. |
| `dereferencePointer` | `Expression` | Produces an expression for the value pointed to by a runtime pointer.                      |
| `addressOf`          | `Expression` | Produces a pointer expression referring to an addressable object.                          |

#### Generic unary and binary operations

| Function      | Returns      | Description                                                |
|---------------|--------------|------------------------------------------------------------|
| `unOp`        | `Expression` | Applies a unary operation and returns a temporary result.  |
| `unOpAssign`  | `Expression` | Applies a unary operation in-place.                        |
| `binOp`       | `Expression` | Applies a binary operation and returns a temporary result. |
| `binOpAssign` | `Expression` | Applies a binary operation in-place to the left-hand side. |

#### Unary operations

| Function        | Returns      | Description                                                        |
|-----------------|--------------|--------------------------------------------------------------------|
| `lnot`          | `Expression` | Computes logical NOT into a temporary result.                      |
| `lnotAssign`    | `Expression` | Applies logical NOT in-place.                                      |
| `lbool`         | `Expression` | Converts a value to a boolean-like `0` or `1` result.              |
| `lboolAssign`   | `Expression` | Converts a value to boolean form in-place.                         |
| `negate`        | `Expression` | Computes arithmetic negation into a temporary result.              |
| `negateAssign`  | `Expression` | Applies arithmetic negation in-place.                              |
| `abs`           | `Expression` | Computes absolute value into a temporary result.                   |
| `absAssign`     | `Expression` | Applies absolute value in-place.                                   |
| `signBit`       | `Expression` | Computes the sign bit of a signed integer into a temporary result. |
| `signBitAssign` | `Expression` | Replaces a signed integer with its sign bit.                       |

#### Arithmetic operations

| Function    | Returns      | Description                                   |
|-------------|--------------|-----------------------------------------------|
| `add`       | `Expression` | Computes `lhs + rhs` into a temporary result. |
| `sub`       | `Expression` | Computes `lhs - rhs` into a temporary result. |
| `mul`       | `Expression` | Computes `lhs * rhs` into a temporary result. |
| `div`       | `Expression` | Computes `lhs / rhs` into a temporary result. |
| `mod`       | `Expression` | Computes `lhs % rhs` into a temporary result. |
| `addAssign` | `Expression` | Applies `lhs += rhs`.                         |
| `subAssign` | `Expression` | Applies `lhs -= rhs`.                         |
| `mulAssign` | `Expression` | Applies `lhs *= rhs`.                         |
| `divAssign` | `Expression` | Applies `lhs /= rhs`.                         |
| `modAssign` | `Expression` | Applies `lhs %= rhs`.                         |

#### Logical operations

| Function      | Returns      | Description                                    |
|---------------|--------------|------------------------------------------------|
| `land`        | `Expression` | Computes logical AND into a temporary result.  |
| `lnand`       | `Expression` | Computes logical NAND into a temporary result. |
| `lor`         | `Expression` | Computes logical OR into a temporary result.   |
| `lnor`        | `Expression` | Computes logical NOR into a temporary result.  |
| `lxor`        | `Expression` | Computes logical XOR into a temporary result.  |
| `lxnor`       | `Expression` | Computes logical XNOR into a temporary result. |
| `landAssign`  | `Expression` | Applies logical AND in-place.                  |
| `lnandAssign` | `Expression` | Applies logical NAND in-place.                 |
| `lorAssign`   | `Expression` | Applies logical OR in-place.                   |
| `lnorAssign`  | `Expression` | Applies logical NOR in-place.                  |
| `lxorAssign`  | `Expression` | Applies logical XOR in-place.                  |
| `lxnorAssign` | `Expression` | Applies logical XNOR in-place.                 |

#### Comparison operations

| Function    | Returns      | Description                                            |
|-------------|--------------|--------------------------------------------------------|
| `eq`        | `Expression` | Computes `lhs == rhs` into a temporary boolean result. |
| `neq`       | `Expression` | Computes `lhs != rhs` into a temporary boolean result. |
| `lt`        | `Expression` | Computes `lhs < rhs` into a temporary boolean result.  |
| `le`        | `Expression` | Computes `lhs <= rhs` into a temporary boolean result. |
| `gt`        | `Expression` | Computes `lhs > rhs` into a temporary boolean result.  |
| `ge`        | `Expression` | Computes `lhs >= rhs` into a temporary boolean result. |
| `eqAssign`  | `Expression` | Applies equality comparison in-place.                  |
| `neqAssign` | `Expression` | Applies inequality comparison in-place.                |
| `ltAssign`  | `Expression` | Applies less-than comparison in-place.                 |
| `leAssign`  | `Expression` | Applies less-or-equal comparison in-place.             |
| `gtAssign`  | `Expression` | Applies greater-than comparison in-place.              |
| `geAssign`  | `Expression` | Applies greater-or-equal comparison in-place.          |

#### Control flow

| Function      | Returns | Description                                                                                                                 |
|---------------|---------|-----------------------------------------------------------------------------------------------------------------------------|
| `label`       | `void`  | Starts a labeled code block. If already inside a block, the previous block is ended and made to fall through to this label. |
| `jump`        | `void`  | Ends the current block and sets the next block to a label.                                                                  |
| `jumpIf`      | `void`  | Ends the current block and selects one of two labels based on an integer condition.                                         |
| `unreachable` | `void`  | Marks the current block as intentionally unreachable.                                                                       |

#### Input/Output
| Function | Returns | Description                                                                                 |
|----------|---------|---------------------------------------------------------------------------------------------|
| `read`   | `void`  | Writes the raw byte or bytes represented by a value.                                        |
| `write`  | `void`  | Writes the raw byte or bytes represented by a value.                                        |
| `print`  | `void`  | Writes a string or integer in decimal notation. Supports unsigned and signed integer types. |


#### `Assembler::ProgramBuilder`

Constructor: `Assembler::program(std::string program_name, std::string entry_function)`.

| Function | Returns | Description                  |
|----------|---------|------------------------------|
| `begin`  | `void`  | Starts program construction. |

Corresponding end-statement: `Assembler::endProgram()`.

#### `Assembler::FunctionBuilder`

Constructor: `Assembler::function(std::string function_name)`.

| Function | Returns           | Description                                                     |
|----------|-------------------|-----------------------------------------------------------------|
| `ret`    | `FunctionBuilder` | Sets the function return type. Defaults to `void` when omitted. |
| `param`  | `FunctionBuilder` | Adds a named parameter to the function.                         |
| `begin`  | `void`            | Defines the function and starts its entry block.                |

Corresponding end-statement: `Assembler::endFunction()`.

#### `Assembler::ScopeBuilder`

Constructor: `Assembler::scope()`.

| Function | Returns | Description                    |
|----------|---------|--------------------------------|
| `begin`  | `void`  | Starts a nested lexical scope. |

Corresponding end-statement: `Assembler::endScope()`.

#### `Assembler::FunctionCallBuilder`

Constructor: `Assembler::callFunction(std::string function_name)`.

| Function | Returns               | Description                                                |
|----------|-----------------------|------------------------------------------------------------|
| `into`   | `FunctionCallBuilder` | Sets the destination for a non-void function return value. |
| `arg`    | `FunctionCallBuilder` | Adds an argument to the call.                              |
| `done`   | `void`                | Finalizes and emits the call.                              |

### Type system API

All type factories live in `acus::ts`.

| Function           | Returns               | Description                                                   |
|--------------------|-----------------------|---------------------------------------------------------------|
| `void_t`           | `ts::TypeHandle`      | Returns the `void` type.                                      |
| `u8`               | `ts::TypeHandle`      | Returns the unsigned 8-bit integer type.                      |
| `s8`               | `ts::TypeHandle`      | Returns the signed 8-bit integer type.                        |
| `u16`              | `ts::TypeHandle`      | Returns the unsigned 16-bit integer type.                     |
| `s16`              | `ts::TypeHandle`      | Returns the signed 16-bit integer type.                       |
| `array`            | `ts::TypeHandle`      | Returns a fixed-length array type.                            |
| `string`           | `ts::TypeHandle`      | Returns a string type with space for a terminating zero byte. |
| `raw`              | `ts::TypeHandle`      | Returns an internal raw storage type of a given slot size.    |
| `struct_t`         | `ts::TypeHandle`      | Looks up an already-defined struct type by name.              |
| `pointer`          | `ts::TypeHandle`      | Returns a runtime pointer type for a pointee type.            |
| `function_pointer` | `ts::TypeHandle`      | Returns a function pointer type for a function type.          |
| `void_function`    | `ts::TypeHandle`      | Returns the `void()` function type.                           |
| `function`         | `FunctionTypeBuilder` | Starts building a function type.                              |
| `defineStruct`     | `StructTypeBuilder`   | Starts defining a named struct type.                          |

#### `ts::FunctionTypeBuilder`

Constructor: `ts::function()`

| Function | Returns                | Description                                            |
|----------|------------------------|--------------------------------------------------------|
| `ret`    | `FunctionTypeBuilder`  | Sets the return type. Defaults to `void` when omitted. |
| `param`  | `FunctionTypeBuilder`  | Adds one parameter type.                               |
| `done`   | `FunctionType const *` | Finalizes and returns the function type.               |

#### `ts::StructTypeBuilder`

Constructor: `ts::defineStruct(std::string struct_name)`

| Function | Returns              | Description                            |
|----------|----------------------|----------------------------------------|
| `field`  | `StructTypeBuilder`  | Adds a named field to the struct type. |
| `done`   | `StructType const *` | Finalizes and returns the struct type. |

### Literal factory API

All literal factories live in `acus::literal`.

| Function           | Returns                | Description                                                                |
|--------------------|------------------------|----------------------------------------------------------------------------|
| `u8`               | `Literal`              | Creates an unsigned 8-bit integer literal.                                 |
| `s8`               | `Literal`              | Creates a signed 8-bit integer literal.                                    |
| `u16`              | `Literal`              | Creates an unsigned 16-bit integer literal.                                |
| `s16`              | `Literal`              | Creates a signed 16-bit integer literal.                                   |
| `string`           | `Literal`              | Creates a string literal.                                                  |
| `function_pointer` | `Literal`              | Creates a function pointer literal referring to a named function.          |
| `struct_t`         | `StructLiteralBuilder` | Starts building a struct literal.                                          |
| `array`            | `ArrayLiteralBuilder`  | Starts building an array literal.                                          |

#### `literal::StructLiteralBuilder`

Constructor: `literal::struct_t(TypeHandle struct_type)`

| Function | Returns                | Description                               |
|----------|------------------------|-------------------------------------------|
| `init`   | `StructLiteralBuilder` | Initializes a named field in the literal. |
| `done`   | `Literal`              | Finalizes and returns the struct literal. |

#### `literal::ArrayLiteralBuilder`

Constructor: `literal::array(TypeHandle array_type)`

| Function | Returns               | Description                              |
|----------|-----------------------|------------------------------------------|
| `push`   | `ArrayLiteralBuilder` | Appends an element literal.              |
| `done`   | `Literal`             | Finalizes and returns the array literal. |

### Operator enums

Generic unary and binary operators use these enums.

| Enum    | Values                                                                                                            | Description                                          |
|---------|-------------------------------------------------------------------------------------------------------------------|------------------------------------------------------|
| `UnOp`  | `Not`, `Bool`, `Neg`, `Abs`, `SignBit`                                                                            | Unary operations used by `unOp` and `unOpAssign`.    |
| `BinOp` | `Add`, `Sub`, `Mul`, `Div`, `Mod`, `And`, `Or`, `Nand`, `Nor`, `Xor`, `Xnor`, `Eq`, `Neq`, `Lt`, `Le`, `Gt`, `Ge` | Binary operations used by `binOp` and `binOpAssign`. |

## License

Acus is licensed under the GNU General Public License v3.0 or later.

See [LICENSE](LICENSE) for the full license text.
