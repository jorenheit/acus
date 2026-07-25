<p align="center">
  <img src="../../../assets/logo_transparent.png" alt="Acus logo" width="180">
</p>

# Acus Sugar

Acus Sugar is a layer of syntactic sugar sprinkled on top of the Acus API, designed to mimic a programming language embedded within C++. It uses templates, operator overloading and macro's to provide a familiar syntax for variables, expressions, functions, loops, conditionals, arrays, structs, pointers, globals, recursion, and basic I/O. 

> [!NOTE]
> Acus Sugar is under active development. The syntax and standard-library API may still change.

## Minimal C++ example
To write a sugar application, the sugar header must be included and it is recommended to import the sugar namespace. There is no need to instantiate an `Assembler` anymore. Instead, the Acus Sugar API provides free functions for many of the Acus API methods. The *Hello World* program from before can now be rewritten to:


```cpp
#include <iostream>
#include <acus/sugar/sugar.h>
using namespace acus::sugar;

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

## Using `acs`
A simple Python script, `acs`, is provided to remove the C++ boilerplate altogether. It accepts source-files containing only the body of the program (the part between `program` and `endProgram`) and will construct the full C++ source-file for you, compile it using a C++ compiler of your choice, then run it to produce the corresponding BF code. 

For example, the *Hello World* snippet from before can be reduced to simply:

```cpp
// hello.acs

function_<void()>("main") | define {
  println("Hello, World!");
  return_;
};
```

After installing `acs` to your path, this program can be compiled to BF using:

```sh
acs hello.acs -o hello.bf
```

## Staring a Program

### Basic Structure
A program is opened with `program()` and finalized with `endProgram()`:

```cpp
program("example", "main");
{
  // Globals and functions
}
endProgram();
```

The second argument is the entry-function name and defaults to `"main"`. After finalization using `endProgram`, retrieve the generated source with:

```cpp
std::string bf = generateBrainfuck("example");
```

Note: these steps can be skipped when using `acs`; it will create them automatically for you so you can concentrate on the body of the program.

### Defining and Declaring Functions
#### Function Definitions
A function is defined using the `function_` 'keyword'. Obviously this is not an actual keyword, but all functions or macro's that behave like keywords have an underscore appended to them to indicate their keyword'ish semantics. For the sake of convenience, such words will be referred to as keywords.

To specify the type of the function (return-type and argument-types), regular C++ function-syntax is used. In the example below, we define a function `add` that takes two `u16` parameters bound to the names `a` and `b` and returns a `u16`. Note the peculiar syntax of the pipe followed by `define` and the body, which needs a semicolon after its closing brace.

```cpp
auto add = function_<u16(u16, u16)>("add", "a", "b") | define {
  return_(var("a") + var("b"));
};
```

The function definition returns a handle, `add` in this case that can be used to call the function:

```cpp
add(10, 20);
```

#### Function Declarations
If a function is defined below the point where it's first used, it must be declared prior to its use. A declaration is very similar to a definition:

```cpp
auto add = function_<u16(u16, u16)>("add", "a", "b") | declare;
```

As long as a function by the name `add` is defined later with the same signature, the handle returned by the declaration can be used to call the function prior to its definition.

### Types and Variables
#### Declaring and Using Variables
Variables can be declared using `let`. The type is then passed as a template parameter and the variable name is passed as a string argument, for example:

```cpp
let<u8>("x"); // declare a local variable "x" of type u8
```

Variables can be initialized immediately if need be:

```cpp
let<u8>("x") = 42; // declare and initialize
```

Once declared, variables can be referred to using `var`:

```cpp
var("x") = var("y") + var("z");
```

#### Integer Types
All Sugar types are compiletime entities, which can therefore be passed as template arguments, as we saw already with `let`. The following integer types are available:

| Type          | Description             |
|---------------|-------------------------|
| `u8`          | Unsigned 8-bit integer  |
| `s8`          | Signed 8-bit integer    |
| `u16`         | Unsigned 16-bit integer |
| `s16`         | Signed 16-bit integer   |

These can be initialized as follows:

```cpp
let<u8>("x") = 1;
let<s16>("y") = -1000;
```

#### Strings
The type `string<N>` stores a string with a fixed maximum capacity:

```cpp
let<string<20>>("name") = "Ada"; // initialize with C-string or std::string 
println(var("name"));
```

#### Arrays

```cpp
using Scores = Array<u8, 4>;

let<Scores>("scores") = Scores{10, 20, 30, 40};
println(var("scores")[2]);

let<u8>("i") = 1;
var("scores")[var("i")] = 99;
```

Arrays support both constant and runtime indexing.

#### Pointers

```cpp
let<u8>("value") = 10;
let<ptr<u8>>("p") = &var("value"); // address-of operator

*var("p") = 20; // dereference operator
println(var("value"));
```

Pointer arithmetic and pointers to aggregate elements are supported where the underlying type rules allow them.


#### User-Defined Types (Structs)
Compound struct-types can also be defined using C++ typedef syntax. For example, to define a `Point` struct, containing two s16 fields, the following syntax can be used:

```cpp
using Point = Struct<"Point",
  Field<"x", s16>,
  Field<"y", s16>
>;
```

With this new type defined, variables can be declared and their fields can be accessed through the field names that were defined before:

```cpp
let<Point>("p") = Point{1, 2};
var("p").field("x") = 3
```

### Operators
The following operators are supported. Their meaning is identical to that when used in C, C++ and similar languages. Semantically, only the logical operators `&&` and `||` are different since they do *not* short-circuit (i.e. both sides are always evaluated).

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

### Control Flow
Below follow some predefined and familiar flow-control mechanisms. At first glance, they look a lot like the ones from C-like languages, but beware of the use of comma's where you might be used to semicolons and the semicolon following the final closing brace. Also, curly braces can not be omitted even when the body contains just a single statement.

#### Conditionals

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

#### While loops

```cpp
while_(var("x") > 0) {
  println(var("x"));
  --var("x");
};
```

#### For loops

```cpp
for_(let<u8>("i") = 0, var("i") < 10, ++var("i")) {
  println(var("i"));
};

#### Break and continue

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

## C++ as a Metaprogramming Language

Acus Sugar is embedded in C++, so it is important to distinguish two kinds of control flow.
Ordinary C++ executes while the Brainfuck program is being generated:

```cpp
for (int i = 0; i < 3; ++i) {
  println("This statement is emitted three times");
}
```
This makes C++ basically the metaprogramming language for an Acus Sugar program (kinda neat, you can do meta-metaprogramming now). Acus Sugar control flow becomes runtime logic in the generated Brainfuck program:

```cpp
for_(let<u8>("i") = 0, var("i") < 3, ++var("i")) {
  println(var("i"));
};
```

The same distinction applies to `if` versus `if_`, and `while` versus `while_`.

## Globals

Globals are declared at program scope and can be referenced from any function:

```cpp
global<string<20>>("message");

function_<void()>("main") | define {
  var("message") = "Hello";
  println(var("message"));
  return_;
};
```

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

## License

Acus and Acus Sugar are licensed under the GNU General Public License v3.0 or later. See `LICENSE` for details.
