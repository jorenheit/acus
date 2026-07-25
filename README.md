<p align="center">
  <img src="assets/logo_transparent.png" alt="Acus logo" width="200">
</p>

# Acus: A Compiler Backend Targeting Brainfuck (C++23)

## Introduction
Acus is primarily intended for building experimental languages and DSLs that target Brainfuck. It's a C++23 library that provides the machinery that a higher-level language frontend can call into: declaring variables, creating functions, pointers, arrays, user-defined types, flow control, etc... Acus lays these constructs out on the Brainfuck tape and lowers them to plain Brainfuck source.

> [!TIP]
> If you just want to get started with generating BF programs, try [Acus Sugar](README-sugar.md). 

### Why this exists
This project is related to [Synapse-191](https://github.com/jorenheit/bfcpu), which aimed to build a Brainfuck computer from scratch (and succeeded). To complete the circle of the Brainfuck computing environment that emerged from this project, a compiler was still a necessary component. That is, a compiler that takes a human-readable and familiar syntax and translates this to Brainfuck, which can then be executed natively on the machine. Please note: Acus is not a compiler! Instead, I set out to build a language-agnostic backend that can be called into by different frontends implementing wildly different languages. 

### Acus Sugar
At the moment of writing, no such frontends exist. However, a thin layer of syntactic sugar was sprinkled on top of Acus (Acus Sugar) to implement a DSL (Domain Specific Language) that feels a lot like a normal programming language, but is embedded in the C++ syntax (making use of operator overloading, template meta-programming and macro's to achieve this). A simple Python script (`acs`), that takes a source-file containing only Acus Sugar syntax, is meant to be used by people who just want to produce BF and not deal with any of the C++ boilerplate. For example, this is a valid `acs` source-file that will produce a BF source-file for the classic Hello World program. More sweet examples are included in the [examples](/frontend/examples) folder.

```cpp
function_<void()>("main") | define {
  println("Hello, World!");
  return_;
};	
```
Visit the [Sugar Readme page here](README-sugar.md).


### Core API: Minimal Example
When writing your own frontend to Acus, you won't be using the Sugar layer. Instead, you'll need to create an `acus::Assembler` object and use its public interface to construct a program. The Sugar example above actually compiles down into the code below, which we will go over in more detail.

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

  std::cout << a.brainfuck("hello");
}
catch (std::exception const &e) {
  std::cerr << e.what() << '\n';
  return 1;
}
```

#### Header and Namespace
To use Acus, you need to include the main acus header: `acus/acus.h`. This will make the necessary types and functions available in the `acus` namespace and several sub-namespaces. Simply import all of the necessary symbols into the global namespace with `using namespace acus::api`. 

#### Program Block
After creating an instance of the `Assembler`, a new program is started. `Acus::program` takes two string arguments: the name of the program and its entry-point. The necessary `.begin()` might look weird at this point, but was added for consistency (more detail below on *builders*). A program block ends at `endProgram`. Note: the curly braces encapsulating the program are only for aestetic reasons.

#### Function Signature
This program contains only a single function: `main`, which was also set as the entry-point for the program. The `Assembler::function` member expects a single string, the name of the function, and returns a builder-object which can be used to add arguments and return-types (more on this later). In this case, our function does not accept arguments and does not return anything, so `begin()` can be called immediately to finalize the signature and start the body.

#### Function Body
The body of `main` contains only two statements: a call to the builtin `print` function and an explicit return from `main`. The print-function takes a string-literal, which is a special type defined in the `literal` namespace. An explicit return is mandatory; in this case it doesn't take any arguments because `main` returns `void`. 

#### Output
After a program has been fully defined (i.e. after `endProgram()`), the corresponding BF program can be requested using `Assembler::brainfuck("program")`. A `std::string` will be returned that contains the full brainfuck source for this program. Multiple programs can be constructed using the same `Assembler` object if necessary.

#### Errors
Acus will try to report useful errors through exceptions. In this example, the exception is simply caught and displayed.

#### Compilation
After installing Acus (making `libacus` and the headers available on the path), the program above can be compiled and run: 

```sh
$ c++ -std=c++23 hello.cc -lacus -o hello
$ ./hello
>[-]+<[-]+>>>>>>>>>>>>>>>>>>>>>>>>>>>[-]+>>[-]>[-]>>>>>>>>[-]+[<<<<<[-]<[-]<
<<[->>>>+<+<<<][-]>>>[-<<<+>>>]<[-]>[-]<<[->+>+<<][-]>>[-<<+>>][-]+>[[-]<[-]
>][-]<[->+<][-]+<[[-]>[-]<][-]>[-<+>][-]>[-<+>]<[[-]<[[-]>>[-]+<<]>]>>>>>>>>
>>[-]<[-]<<<[->>>>+<+<<<][-]>>>[-<<<+>>>]<[-]>[-]<<<<<<<<[->>>>>>>+>+<<<<<<<
<][-]>>>>>>>>[-<<<<<<<<+>>>>>>>>][-]>[-<+>]<[[-]<[[-]>>[-]+<<]>]<<<<<<<<[-]>
>>>>>>>>[[-]>>>>>[-]>>++++++++[-<<+++++++++>>]<<.+++++++++++++++++++++++++++
++.+++++++..+++.------------------------------------------------------------
-------.------------.+++++++++++++++++++++++++++++++++++++++++++++++++++++++
.++++++++++++++++++++++++.+++.------.--------.------------------------------
-------------------------------------.-----------------------.[-]<<<<<<<<<[-
]>>>>]<<<<<<<<<[-]<[-]<<<[->>>>+<+<<<][-]>>>[-<<<+>>>]<[-]>[-]<<[->+>+<<][-]
>>[-<<+>>]>-<[-]+>[[-]<[-]>][-]<[->+<][-]+<[[-]>[-]<][-]>[-<+>][-]>[-<+>]<[[
-]<[[-]>>[-]+<<]>]>>>>>>>>>>[-]<[-]<<<[->>>>+<+<<<][-]>>>[-<<<+>>>]<[-]>[-]<
<<<<<<<[->>>>>>>+>+<<<<<<<<][-]>>>>>>>>[-<<<<<<<<+>>>>>>>>][-]>[-<+>]<[[-]<[
[-]>>[-]+<<]>]<<<<<<<<[-]>>>>>>>>>[[-]]<<<<]
```

The result is a lot less efficient when compared to a dedicated Hello World implementation like the one found on [Wikipedia](https://en.wikipedia.org/wiki/Brainfuck):
```sh
++++++++[>++++[>++>+++>+++>+<<<<-]>+>+>->>+[<]<-]>>.>---.+++++++..+++.>>.<-.
<.+++.------.--------.>>+.>++.
```

This is the tradeoff that needs to be made when using Acus: generality vs efficiency. Acus tries to optimize its output as much as possible, but it still needs to setup the machinery to allow for jumps the simulation of random access memory (to implement pointers). You can actually se the boiler-plate for this in the output above: the first sequence of eight `+` commands is where the construction of the *Hello World* string actually begins. Everything before that is related to the setup of a 'virtual machine'. If you're interested in the technical details of the Acus implementation, please refer to the report in the [Synapse-191 repository](https://github.com/jorenheit/bfcpu).

## Acus Component Breakdown

The table below shows the classes and namespaces available after `acus.h` has been included (full path) and, optionally, when the `acus::api` namespace is imported (short path):

| Full path                | Short path         | type       | Description                                         |
|--------------------------|--------------------|------------|-----------------------------------------------------|
| `acus::Assembler`        | `Assembler`        | class      | The main assembler class used to generate BF.       |
| `acus::ts`               | `ts`               | namespace  | Typesystem namespace, used to request type-handles. |
| `acus::ts::TypeHandle`   | `TypeHandle`       | class      | Object representing a type.                         |
| `acus::literal`          | `literal`          | namespace  | Namespace for literal-generation.                   |
| `acus::literal::Literal` | `Literal`          | class      | Literal object, representing a compile-time value.  |
| `acus::Expression`       | `Expression`       | class      | Object representing expressions.                    |
| `acus::error::Error`     | `error::Error`     | class      | Error object, thrown when an error occurs.          |
| `acus::error::ErrorCode` | `error::ErrorCode` | enum class | Error code embedded in the Error object.            |

Each of these will be described in more detail below.

### Assembler
`acus::Assembler` is the main public entry point. It owns the program under construction and exposes methods for declaring data, defining functions, creating expressions, emitting control flow, and generating Brainfuck. The full API is listed at the bottom of this page.

A typical program has this shape:

```cpp
// 1. Instantiate the Assembler object
Assembler a;

// 2. Start a new program
a.program("program_name", "main").begin();

/* 
    Optional: global variable declarations using Assembler::declareGlobal.
*/

// 3. Entrypoint function (not necessarily the first one)
a.function("main").begin(); 

/* 
	Function body 
*/

// 4. Mandatory explicit return
a.returnFromFunction(); 

// 5. End of the function definition
a.endFunction();
  
/*
	Optional: other functions.
*/

// 6. End of the program definition
a.endProgram();

// 7. Request the BF code
std::string bf = c.brainfuck("program_name");
```

### Typesystem and Variables
#### Builtin Types
The typesystem `acus::ts` is used to acquire `TypeHandle` objects representing types. These handles can be passed to the `Assembler` when, for instance, new variables or functions are being declared. Acus supports the following builtin types:

| Type          | Description                   | Typesystem Call   |
|---------------|-------------------------------|-------------------|
| `u8`          | Unsigned 8-bit integer        | `ts::u8()`        |
| `u16`         | Unsigned 16-bit integer       | `ts::u16()`       |
| `s8`          | Signed 8-bit integer          | `ts::s8()`        |
| `s16`         | Signed 16-bit integer         | `ts::s16()`       |
| `array(T, N)` | Array of N elements of type T | `ts::array(T, N)` |
| `pointer(T)`  | Pointer to data of type T     | `ts::pointer(T)`  |

	In these calls, `T` must be a `TypeHandle`. For example, to define a type for an array of 10 `u8` integers, one could define it like so:

```cpp
TypeHandle arrT = ts::array(ts::u8(), 10);
```

#### User-Defined Types
In addition to builtin types, compound types (structs) can be defined as well. The function `ts::defineStruct` takes the name of the type and returns a builder object of type `ts::StructTypeBuilder`. This object is used to incrementally define the type by adding named fields to it. When all fields have been added, `done()` finalizes the type and returns a handle.

```cpp
// In one go:
TypeHandle pointT = ts::defineStruct("Point")
	.field("x", ts::s16())
	.field("y", ts::s16())
	.done();
	
// Or spread out:
ts::StructTypeBuilder builder = ts::defineStruct("Point");
builder.field("x", ts::s16());
builder.field("y", ts::s16());
TypeHandle pointT = builder.done();
```

#### Declaring Variables
Variables can be declared through `Assembler::declareLocal` and `Assembler::declareGlobal`. The first is used in function-scope and returns an expression to the declared variable. The latter must be used before any functions are defined and does not return an expression. If you need an expression object to represent a global variable, the `Assembler::expr` function can be used (at function scope) to produce one.

```cpp
// At global scope:
a.declareGlobal("g", ts::u8());

// At function scope:
Expression x = a.declareLocal("x", ts::u8()); 

a.print(a.add(x, "g"));   // Option 1: use expression object to refer to x
a.print(a.add("x", "g")); // Option 2: use variable name to refer to x

// Using an expression object for g
Expression g = a.expr("g");
a.print(a.add(x, g));
```

### Expressions and Operators

All `Assembler` functions that act as operators accept a common expression abstraction as their operands: `acus::Expression`. An expression may refer to a local or global variable, a literal, an array element, a struct field, a dereferenced pointer, or the result of another operation. For example, addition of two variables returns a new expression that refers to a temporary containing the result of the addition.

```cpp
// Declare 3 local u8 variables
TypeHandle u8 = ts::u8();
Expression x = a.declareLocal("x", u8); // store a handle to "x"
Expression y = a.declareLocal("y", u8);
a.declareLocal("z", u8);                // not stored

// Assign values to x and y
a.assign(x, literal::u8(24));           // use the expression directly
a.assign(y, literal::u8(18));

// Compute x + y and assign to z
Expression result = a.add(x, y);
a.assign("z", result);                  // refer to z by name

// Or equivalently
a.assign("z", a.add("x", "y"));         // only use names

/* !! Don't use 'result' after the assignment !! */
```

#### Temporaries and Undefined Behavior
In the example above, different approaches were used. When declaring a local variable, the result is an `Expression` referring to the result of that declaration (the variable itself). When this expression is stored, it can later be passed to operators like `add`. The same is true for the result of such an operator: the `Expression` object returned by an operator represents the result after application of that operator. However, in this case the result is not bound to any variables and is therefore stored in a temporary object. Passing a temporary to `assign` will cause it to be consumed (move-semantics) and become invalid after the assignment, even if you still own the result-object. Using expressions beyond their scope is therefore undefined behavior.

### Errors
When Acus encounters an ill-defined program, it will throw an exception containing details about the error, including file and line information referring to the C++ source where the error was made. Exceptions thrown by Acus all derive from `std::exception`, so the easiest way to deal with them is to simply catch them, print the message and exit the program.

```cpp
try {
  ...
} catch (std::exception &e) {
  std::cerr << e.what() << '\n';
  std::exit(1);
}
```

For more advanced handling of errors, they need to be caught as `error::Error` objects. These give access to their error-code and allow for relocation (i.e. changing their file and line information). The example below prints the error-code, relocates the error and throws it to be handled elsewhere.

```cpp
try {
  ...
} catch (error::Error &e) {
  std::cerr << "Code: " << e.code() << '\n';
  e.relocate("source.src", line, col);
  throw;
}
```

A full table of error-codes is shown at the bottom of this page.


## Control Flow
### Labels and Jumps
Acus exposes a basic system of labels and jumps to implement flow-control, rather providing loops and conditionals. This makes it suitable for many different frontends without making assumptions about language structure. The snippet below shows how a for-loop could be implemented using labels and jumps:

```cpp
// for (i = 0; i < n; ++i) { ... }

a.assign("i", literal::u8(0));

a.label("check");
a.jumpIf(a.lt("i", "n"), "body", "done");

a.label("body");
// ...
a.addAssign("i", literal::u8(1));
a.jump("check");

a.label("done");
```
### Functions and calls
#### Function Definitions
Both functions and calls use builders to construct them. A function definition is started by acquiring a builder throught the `Assembler::function` method. This returns an `Assembler::FunctionBuilder` object, exposing its `param`, `ret` and `begin` functions. The `param` function can be called repeatedly on the builder: once for every parameter the function takes. Calling `ret` is optional and is only necessary for functions that return non-void. When the signature is fully defined, `begin` is called and the function-scope is entered. 

Each function is closed by `Assembler::endFunction`. Before doing so, each possible path through the function must terminate in either a call to `Assembler::returnFromFunction` or `Assembler::abortProgram`.

```cpp
a.function("double")
  .param("x", ts::u16())
  .ret(ts::u16())
  .begin(); {
    a.returnFromFunction(a.mul("x", literal::u16(2)));
  } a.endFunction();

// Or equivalently:
Assembler::FunctionBuilder builder = a.function("double");
builder.param("x", ts::u16());
builder.ret(ts::u16());
builder.begin();
a.returnFromFunction(a.mul("x", literal::u16(2));
a.endFunction();
```

#### Function Calls
Calls also use a builder to set them up properly. The `Assembler::callFunction` takes the function-name as its only parameter and returns an `Assembler::FunctionCallBuilder` object, exposing its members `arg` (to pass an argument), `into` (to specify a return target) and `done` to finalize and execute the call.

```cpp
a.callFunction("double")
  .arg("x")
  .into("y")
  .done();
  
// Or equivalently:
Assembler::FunctionCallBuilder builder = a.callFunction("double");
builder.arg("x");
builder.into("y");
buider.done();
```

#### Calling a Function Pointer
Calls can also be done dynamically using a function pointer. The syntax is equivalent to that of a normal call, except that it takes an expression representing a function-pointer instead of a string. In order to define a function-pointer type, one first must define the function-type, from which the function-pointer-type can be defined. A variable of this type can then be declared and assigned a `function_pointer` literal. Depending on runtime conditions, different functions can be assigned to the pointer, resulting in dynamic dispatch.

```cpp

// 1. Define the function-type
TypeHandle fooType = ts::function()
	.param(ts::s16())
	.ret(ts::s16())
	.done();

// 2. Construct function pointers as literals
Literal doubleFunction = literal::function_pointer(fooType, "double");
Literal tripleFunction = literal::function_pointer(fooType, "triple");

// 3. Define function-pointer type
auto fooPtrType = ts::function_pointer(fooType);

// 4. Declare the function pointer
c.declareLocal("fPtr", fooPtrType);

// 5. Ask user for input:
c.declareLocal("input", ts::s16());
c.print(literal::string("Enter 'd' or 't': "));
c.read("input");

// 6. Branch based on the input
c.jumpIf(c.eq("input", literal::u8('d')), "useDouble", "useTriple");

// 7. Branch 1
c.label("useDouble");
c.assign("fPtr", doubleFunction);
c.jump("done");

// 8. Branch 2
c.label("useTriple");
c.assign("fPtr", tripleFunction);

// 9. Call through pointer and print result
c.label("done");
c.declareLocal("x", ts::s16());
c.assign("x", literal::s16(21));
c.callFunctionPointer("fPtr")
  .arg("x")
  .into("x")
  .done();

c.print("x");
```

## Public API reference

The tables below describe the supported user-facing backend API exposed by `<acus/acus.h>`. For readability, the final optional `std::source_location` argument accepted by API functions is omitted from the signatures. Low-level storage, proxy, cache, and primitive-IR types that are transitively visible through the implementation headers are not treated as stable frontend API here.

Most `Assembler` operands may be supplied as a variable name, an `Expression`, a `Literal`, or—at the lower level—a `SlotProxy`. Operations requiring an lvalue accept only an addressable variable or expression.

<details>
<summary><strong>Expand the complete API tables</strong></summary>

### Names imported by `acus::api`

After:

```cpp
using namespace acus::api;
```

these names are available directly:

| Imported name | Original name | Purpose |
|---|---|---|
| `Assembler` | `acus::Assembler` | Constructs programs and emits Brainfuck. |
| `Expression` | `acus::Expression` | Represents a literal value or an addressable runtime value. |
| `TypeHandle` | `acus::ts::TypeHandle` | Handle to an Acus type. |
| `Literal` | `acus::literal::Literal` | Compile-time literal value. |
| `ts` | `acus::ts` | Type factories and type builders. |
| `literal` | `acus::literal` | Literal factories and literal builders. |

`acus::error`, `acus::types`, `acus::BinOp`, and `acus::UnOp` are not re-exported into `acus::api`; use their fully qualified names or import them separately.

### `acus::Assembler`

#### Construction and generated output

| Function | Returns | Description |
|---|---|---|
| `Assembler()` | — | Constructs an independent assembler instance. The same instance may generate multiple named programs sequentially. |
| `primitives(name)` | `std::string` | Returns the textual primitive-IR listing stored for a completed program. Throws `NoSuchProgram` if the name is unknown. |
| `brainfuck(name)` | `std::string` | Returns the generated Brainfuck source stored for a completed program. Throws `NoSuchProgram` if the name is unknown. |

#### Program, function, and scope structure

| Function | Returns | Description |
|---|---|---|
| `program(name, entry)` | `ProgramBuilder` | Starts configuring a named program and its entry function. Finalize the builder with `begin()`. |
| `function(name)` | `FunctionBuilder` | Starts configuring a function definition. Finalize with `begin()`. |
| `scope()` | `ScopeBuilder` | Starts configuring a nested lexical scope. Finalize with `begin()`. |
| `endProgram()` | `void` | Validates and completes the current program, emits support blocks, and stores its primitive and Brainfuck output. |
| `endFunction()` | `void` | Ends the current function after all nested scopes have been closed. |
| `endScope()` | `void` | Ends the current lexical scope and releases its local storage for reuse. |

#### Declarations

| Function | Returns | Description |
|---|---|---|
| `declareLocal(name, type)` | `Expression` | Declares a local in the current function/scope and returns its addressable expression. |
| `declareGlobal(name, type)` | `void` | Declares a program-global object. Globals may subsequently be referenced by name from functions. |

#### Function calls, returns, and termination

| Function | Returns | Description |
|---|---|---|
| `callFunction(name)` | `FunctionCallBuilder` | Starts a direct call. The callee may be defined later; validation is deferred until `endProgram()`. |
| `callFunctionPointer(pointer)` | `FunctionCallBuilder` | Starts an indirect call through a function-pointer expression. |
| `returnFromFunction()` | `void` | Returns from a `void` function. |
| `returnFromFunction(value)` | `void` | Returns a value compatible with the current function's declared return type. |
| `abortProgram()` | `void` | Emits a path that terminates the entire generated program immediately. |

#### Expression construction, assignment, and access

| Function | Returns | Description |
|---|---|---|
| `expr(value)` | `Expression` | Converts an addressable operand into an `Expression`. Unlike ordinary rvalue conversion, literals are not accepted. |
| `assign(lhs, rhs)` | `Expression` | Assigns `rhs` to an addressable `lhs` and returns the destination expression. |
| `cast(value, type)` | `Expression` | Converts an addressable integer expression to another integer type and returns a temporary result. |
| `structField(object, fieldName)` | `Expression` | Selects a struct field by name. Works for runtime expressions and struct literals. |
| `structField(object, fieldIndex)` | `Expression` | Selects a struct field by zero-based index. |
| `arrayElement(array, constantIndex)` | `Expression` | Selects an array or string element using a compile-time index. |
| `arrayElement(array, runtimeIndex)` | `Expression` | Selects an array or string element using an integer expression. |
| `dereferencePointer(pointer)` | `Expression` | Produces an addressable expression for the pointee of a runtime pointer. |
| `addressOf(object)` | `Expression` | Produces a pointer to an addressable non-temporary object. |

#### Generic unary and binary dispatch

| Function | Returns | Description |
|---|---|---|
| `unOp(op, value)` | `Expression` | Applies an `UnOp` and returns a temporary result. |
| `unOpAssign(op, value)` | `Expression` | Applies an `UnOp` in place to an addressable operand. |
| `binOp(op, lhs, rhs)` | `Expression` | Applies a `BinOp` and returns a temporary result. |
| `binOpAssign(op, lhs, rhs)` | `Expression` | Applies a `BinOp` in place to the addressable left operand. |

#### Unary operations

| Temporary-result function | In-place function | Description |
|---|---|---|
| `lnot(value)` | `lnotAssign(value)` | Logical NOT; produces canonical `0` or `1`. |
| `lbool(value)` | `lboolAssign(value)` | Converts an integer to canonical Boolean form. |
| `negate(value)` | `negateAssign(value)` | Arithmetic negation. |
| `abs(value)` | `absAssign(value)` | Absolute value. |
| `signBit(value)` | `signBitAssign(value)` | Extracts the sign bit of a signed integer as `0` or `1`. |

#### Arithmetic operations

| Temporary-result function | In-place function | Description |
|---|---|---|
| `add(lhs, rhs)` | `addAssign(lhs, rhs)` | Addition / `+=`. Also supports the pointer-plus-integer combinations permitted by the type rules. |
| `sub(lhs, rhs)` | `subAssign(lhs, rhs)` | Subtraction / `-=`. |
| `mul(lhs, rhs)` | `mulAssign(lhs, rhs)` | Multiplication / `*=`. |
| `div(lhs, rhs)` | `divAssign(lhs, rhs)` | Integer division / `/=`. |
| `mod(lhs, rhs)` | `modAssign(lhs, rhs)` | Integer remainder / `%=`. |

#### Logical operations

These are integer truth-value operations; they do not provide host-language short-circuit evaluation.

| Temporary-result function | In-place function | Description |
|---|---|---|
| `land(lhs, rhs)` | `landAssign(lhs, rhs)` | Logical AND. |
| `lnand(lhs, rhs)` | `lnandAssign(lhs, rhs)` | Logical NAND. |
| `lor(lhs, rhs)` | `lorAssign(lhs, rhs)` | Logical OR. |
| `lnor(lhs, rhs)` | `lnorAssign(lhs, rhs)` | Logical NOR. |
| `lxor(lhs, rhs)` | `lxorAssign(lhs, rhs)` | Logical XOR. |
| `lxnor(lhs, rhs)` | `lxnorAssign(lhs, rhs)` | Logical XNOR. |

#### Comparison operations

| Temporary-result function | In-place function | Description |
|---|---|---|
| `eq(lhs, rhs)` | `eqAssign(lhs, rhs)` | Equality comparison. |
| `neq(lhs, rhs)` | `neqAssign(lhs, rhs)` | Inequality comparison. |
| `lt(lhs, rhs)` | `ltAssign(lhs, rhs)` | Less-than comparison. |
| `le(lhs, rhs)` | `leAssign(lhs, rhs)` | Less-than-or-equal comparison. |
| `gt(lhs, rhs)` | `gtAssign(lhs, rhs)` | Greater-than comparison. |
| `ge(lhs, rhs)` | `geAssign(lhs, rhs)` | Greater-than-or-equal comparison. |

Comparison results are canonical `u8` Boolean values. The in-place forms replace the left operand with that result.

#### Explicit control flow

| Function | Returns | Description |
|---|---|---|
| `label(name)` | `void` | Begins a labeled block. If a current block exists, it falls through to this label. |
| `jump(label)` | `void` | Ends the current block and transfers control unconditionally. |
| `jumpIf(condition, trueLabel, falseLabel)` | `void` | Ends the current block and selects one of two labels according to an integer condition. |
| `unreachable()` | `void` | Marks the current block as intentionally unreachable, suppressing the unreachable-block validation error. |

#### Input and output

| Function | Returns | Description |
|---|---|---|
| `read(destination)` | `void` | Reads one Brainfuck input byte into an addressable value whose storage size is one slot. |
| `write(value)` | `void` | Emits the raw encoded byte fields of a value rather than formatting it. |
| `print(value)` | `void` | Prints an integer in decimal notation or a null-terminated Acus string. |

### Builder objects

All builders are move-only and are intended to be finalized exactly once using the method shown below. Destroying an unfinished builder reports `BuilderNotFinalized`; checked configuration methods reject use after finalization with `BuilderUsedAfterFinalize`. Configuration methods support both stored builders and fluent chaining by returning an lvalue or rvalue reference to the same builder.

| Factory | Builder | Configuration methods | Finalizer | Result/effect |
|---|---|---|---|---|
| `Assembler::program(name, entry)` | `Assembler::ProgramBuilder` | — | `begin()` | Begins a program. |
| `Assembler::function(name)` | `Assembler::FunctionBuilder` | `param(name, type)`, `ret(type)` | `begin()` | Defines and enters a function. |
| `Assembler::scope()` | `Assembler::ScopeBuilder` | — | `begin()` | Enters a lexical scope. |
| `Assembler::callFunction(name)` | `Assembler::FunctionCallBuilder` | `arg(value)`, `into(destination)` | `done()` | Emits a direct call. |
| `Assembler::callFunctionPointer(ptr)` | `Assembler::FunctionCallBuilder` | `arg(value)`, `into(destination)` | `done()` | Emits an indirect call. |
| `ts::function()` | `ts::FunctionTypeBuilder` | `param(type)`, `ret(type)` | `done()` | Returns `types::FunctionType const *`. |
| `ts::defineStruct(name)` | `ts::StructTypeBuilder` | `field(name, type)` | `done()` | Returns `types::StructType const *`. |
| `literal::struct_t(type)` | `literal::StructLiteralBuilder` | `init(field, literal)` | `done()` | Returns a completed `Literal`. |
| `literal::array(type)` | `literal::ArrayLiteralBuilder` | `push(literal)` | `done()` | Returns a completed `Literal`. |

#### `Assembler::ProgramBuilder`

| Method | Returns | Description |
|---|---|---|
| `begin()` | `void` | Finalizes the builder and begins construction of the named program. |

Close the corresponding program with `Assembler::endProgram()`.

#### `Assembler::FunctionBuilder`

| Method | Returns | Description |
|---|---|---|
| `param(name, type)` | same builder | Appends a named parameter in declaration order. |
| `ret(type)` | same builder | Sets the return type. It may be specified at most once; omission means `void`. |
| `begin()` | `void` | Finalizes the signature, defines the function, and enters its initial block. |

Close the function with `Assembler::endFunction()`.

#### `Assembler::ScopeBuilder`

| Method | Returns | Description |
|---|---|---|
| `begin()` | `void` | Finalizes the builder and enters a nested lexical scope. |

Close the scope with `Assembler::endScope()`.

#### `Assembler::FunctionCallBuilder`

| Method | Returns | Description |
|---|---|---|
| `arg(value)` | same builder | Appends one argument in parameter order. |
| `into(destination)` | same builder | Selects an addressable destination for a non-void return value. |
| `done()` | `void` | Validates/finalizes the builder and emits the call. |

#### `ts::FunctionTypeBuilder`

| Method | Returns | Description |
|---|---|---|
| `param(type)` | same builder | Appends one parameter type. |
| `ret(type)` | same builder | Sets the return type at most once; omission means `void`. |
| `done()` | `types::FunctionType const *` | Interns and returns the completed function type. |

#### `ts::StructTypeBuilder`

| Method | Returns | Description |
|---|---|---|
| `field(name, type)` | same builder | Appends a named field in layout order. |
| `done()` | `types::StructType const *` | Defines and returns the struct type. Returns `nullptr` if that struct name already exists. |

#### `literal::StructLiteralBuilder`

| Method | Returns | Description |
|---|---|---|
| `init(field, literal)` | same builder | Initializes one named field. Each field may be initialized only once. |
| `done()` | `Literal` | Validates field coverage/types and returns the completed struct literal. |

#### `literal::ArrayLiteralBuilder`

| Method | Returns | Description |
|---|---|---|
| `push(literal)` | same builder | Appends one element and checks its exact element type. |
| `done()` | `Literal` | Requires exactly the array type's declared number of elements and returns the completed literal. |

### `acus::Expression`

`Expression` objects are normally obtained from `Assembler` operations rather than constructed directly.

| Method | Returns | Description |
|---|---|---|
| `hasSlot()` | `bool` | Reports whether the expression refers to runtime/addressable storage. |
| `isLiteral()` | `bool` | Reports whether the expression contains a compile-time literal. |
| `slot()` | `SlotProxy const &` | Returns the underlying storage proxy; valid only when `hasSlot()` is true. This is a lower-level interface. |
| `literal()` | `Literal` | Returns the contained literal; valid only when `isLiteral()` is true. |
| `type()` | `types::TypeHandle` | Returns the expression's static type. |
| `str()` | `std::string` | Returns a diagnostic representation of the expression. |

Public explicit constructors also exist for `Slot`, `SlotProxy`, and `Literal`, primarily for backend/library implementation.

### Type system API

All type factories live in `acus::ts`. Returned objects are interned and remain valid for the lifetime of the process.

#### Type factories

| Function | Returns | Description |
|---|---|---|
| `void_t()` | `types::VoidType const *` | Returns the `void` type. |
| `u8()` | `types::IntegerType const *` | Returns unsigned 8-bit integer type. |
| `s8()` | `types::IntegerType const *` | Returns signed 8-bit integer type. |
| `u16()` | `types::IntegerType const *` | Returns unsigned 16-bit integer type. |
| `s16()` | `types::IntegerType const *` | Returns signed 16-bit integer type. |
| `array(elementType, length)` | `types::ArrayType const *` | Returns a fixed-length array type. |
| `string(maxLength)` | `types::StringType const *` | Returns a string with `maxLength` usable characters plus its terminating zero element. |
| `raw(slotCount)` | `types::RawType const *` | Returns an untyped raw-storage type. This is chiefly useful to backend/library code. |
| `struct_t(name)` | `types::StructType const *` | Looks up a previously defined struct, returning `nullptr` when absent. |
| `pointer(pointeeType)` | `types::PointerType const *` | Returns a runtime pointer type. |
| `function_pointer(functionType)` | `types::FunctionPointerType const *` | Returns a pointer type for the supplied function signature. |
| `void_function()` | `types::FunctionType const *` | Returns the canonical `void()` function type. |
| `function()` | `FunctionTypeBuilder` | Starts building a function type. |
| `defineStruct(name)` | `StructTypeBuilder` | Starts defining a named struct type. |

`ts::TypeHandle` is an alias for `types::Type const *`. `types::null` is the null type handle used internally and by builders before a type is selected.

#### Common type inspection

| Type | Public methods | Description |
|---|---|---|
| `types::Type` | `tag()`, `str()`, `size()`, `usesValue1()` | Base interface: category tag, printable name, number of macrocells, and whether a second value byte is used. |
| `types::IntegerType` | `bits()`, `isSigned()`, `signedness()` | Integer width and signedness. |
| `types::ArrayLike` | `length()`, `elementType()` | Shared interface for arrays and strings. A string's reported length includes its terminating element. |
| `types::StructType` | `fieldCount()`, `fieldIndex(name)`, `fieldType(index/name)`, `fieldOffset(index/name)`, `fieldName(index)` | Struct field metadata and layout. |
| `types::PointerType` | `pointeeType()` | Returns the pointed-to type. |
| `types::FunctionType` | `returnType()`, `paramTypes()` | Returns the function result and ordered parameter types. |
| `types::FunctionPointerType` | `functionType()` | Returns the pointed-to function signature. |

#### Type helpers and enums

| API | Values / result | Description |
|---|---|---|
| `types::cast<T>(handle)` | `T const *` | Downcasts a type handle; an invalid cast triggers an assertion. |
| `types::isU8`, `isS8`, `isU16`, `isS16` | `bool` | Tests a concrete integer type. |
| `types::isInteger` | `bool` | Tests any supported integer type. |
| `types::isSignedInteger`, `isUnsignedInteger` | `bool` | Tests integer signedness category. |
| `types::isArray`, `isString`, `isArrayLike` | `bool` | Tests aggregate sequence categories. |
| `types::isStruct`, `isPointer`, `isFunction`, `isFunctionPointer`, `isRaw` | `bool` | Tests the remaining type categories. |
| `types::TypeTag` | `VOID`, `RAW`, `U8`, `S8`, `U16`, `S16`, `ARRAY`, `STRING`, `STRUCT`, `POINTER`, `FUNCTION`, `FUNCTION_POINTER` | Values returned by `Type::tag()`. |
| `types::Signedness` | `SIGNED`, `UNSIGNED` | Values returned by `IntegerType::signedness()`. |

### Literal API

All literal factories live in `acus::literal`.

#### Literal factories

| Function | Returns | Description |
|---|---|---|
| `u8(value)` | `Literal` | Creates an unsigned 8-bit integer literal. |
| `s8(value)` | `Literal` | Creates a signed 8-bit integer literal. |
| `u16(value)` | `Literal` | Creates an unsigned 16-bit integer literal. |
| `s16(value)` | `Literal` | Creates a signed 16-bit integer literal. |
| `string(value)` | `Literal` | Creates a null-terminated string literal. |
| `function_pointer(functionType, functionName)` | `Literal` | Creates a function-pointer literal naming a function with the supplied signature. |
| `struct_t(structType)` | `StructLiteralBuilder` | Starts building a struct literal. |
| `array(arrayType)` | `ArrayLiteralBuilder` | Starts building an array literal. |
| `cast<T>(literal)` | implementation view | Retrieves typed literal data for supported type categories. An invalid cast triggers an assertion. |

#### `literal::Literal`

| Method | Returns | Description |
|---|---|---|
| `clone()` | `Literal` | Deep-clones the literal value. |
| `str()` | `std::string` | Returns a diagnostic/source-like representation. |
| `type()` | `types::TypeHandle` | Returns the literal's static type. |
| `get()` | shared implementation pointer | Exposes the underlying literal object; generally prefer `literal::cast<T>()`. |

Supported `literal::cast<T>()` categories expose these useful methods:

| Cast category | Available data |
|---|---|
| `types::IntegerType` | `semanticValue()`, `encodedValue()` |
| `types::ArrayLike` / `types::ArrayType` | `element(index)` |
| `types::StringType` | `element(index)`, `stdstr()` |
| `types::StructType` | `field(name)`, `field(index)` |
| `types::FunctionPointerType` | `functionName()` |

### Operator API

| API | Values / return | Description |
|---|---|---|
| `acus::UnOp` | `Not`, `Bool`, `Neg`, `Abs`, `SignBit` | Unary operators accepted by `unOp()` and `unOpAssign()`. |
| `acus::BinOp` | `Add`, `Sub`, `Mul`, `Div`, `Mod`, `And`, `Or`, `Nand`, `Nor`, `Xor`, `Xnor`, `Eq`, `Neq`, `Lt`, `Le`, `Gt`, `Ge` | Binary operators accepted by `binOp()` and `binOpAssign()`. |
| `acus::unOpStr(op)` | `std::string` | Returns the diagnostic name/symbol for a unary operator. |
| `acus::binOpStr(op)` | `std::string` | Returns the diagnostic name/symbol for a binary operator. |

### Error API

Acus reports invalid API use and invalid generated programs by throwing `acus::error::Error`, which derives from `std::exception`. Most public API calls capture their call site using a defaulted `std::source_location`, so diagnostics normally point to the frontend statement that requested the operation.

#### `acus::error::Error`

| Member | Type / return | Description |
|---|---|---|
| `errorCode` | `ErrorCode` | Public stored error code. Prefer `code()` when only reading it. |
| `filename` | `std::string` | Source filename associated with the error. |
| `line` | `int` | One-based source line. |
| `column` | `int` | One-based source column when available. |
| `msg` | `std::string` | Error message without the location prefix. |
| `code()` | `ErrorCode` | Returns the error code. |
| `relocate(file, line, column)` | `Error &` | Replaces the source location and returns the same exception object. Useful for frontends translating locations. |
| `what()` | `char const *` | Returns `file:line:column: message`. |

`error::throw_if(condition, code, file, line, column, ...)` is also public and throws an `Error` assembled from the remaining streamable arguments when `condition` is true.

#### `acus::error::ErrorCode`

##### API sequencing and generated-program structure

| Code | Meaning |
|---|---|
| `UnexpectedApiCall` | An API method was called out of the sequence expected by the backend. |
| `EmptyProgram` | A program was finalized without any functions. |
| `EntryFunctionNotDefined` | The named entry function was never defined. |
| `WrongEntryFunctionType` | The entry function does not have the required `void()` signature. |
| `ExecutionPathWithoutReturn` | A reachable function path ends without returning or aborting. |
| `UnreachableCodeSection` | A block is unreachable but was not explicitly marked with `unreachable()`. |
| `ExpectedOutsideProgramBlock` | An operation requiring no active program was called inside one. |
| `ExpectedInsideProgramBlock` | An operation requiring an active program was called outside one. |
| `ExpectedOutsideFunctionBlock` | An operation requiring no active function was called inside one. |
| `ExpectedInsideFunctionBlock` | An operation requiring an active function was called outside one. |
| `ExpectedNoScope` | An operation was called while nested scopes were still active. |
| `GlobalDeclarationNotAllowed` | A global was declared after the program had entered a phase where globals are no longer accepted. |
| `NoSuchProgram` | `brainfuck()` or `primitives()` was asked for an unknown/completely unfinalized program. |

##### Names, functions, labels, and control-flow targets

| Code | Meaning |
|---|---|
| `DuplicateFunctionParameters` | A function signature contains repeated parameter names. |
| `DuplicateGlobalReferences` | Duplicate global references were requested. This code remains declared although the current direct API no longer exposes `referGlobals()`. |
| `FunctionDoesNotExist` | A direct call or validation references a function that was never defined. |
| `LabelDoesNotExist` | A jump references a label that was never defined in the function. |
| `GlobalNameUnavailable` | A global name is already in use. |
| `ExpectedGlobal` | An operation expected the supplied name to identify a global. |
| `NameNotInScope` | A variable name cannot be resolved in the current local/global scope. |
| `NameAlreadyInScope` | A declaration conflicts with a visible name. |
| `NameAlreadyInCurrentScope` | A declaration duplicates a name in the current lexical scope. |

##### Expressions, operands, and type requirements

| Code | Meaning |
|---|---|
| `TakingAddressOfTemporary` | `addressOf()` was applied to temporary storage. |
| `ReadOnlyExpression` | An operation requiring an lvalue received a literal/read-only expression. |
| `IncompatibleOperands` | An operator or cast is not defined for the supplied types/order. |
| `NotPrintable` | `print()` does not support the supplied type. |
| `ExpectedInteger` | An integer type was required. |
| `ExpectedSignedInteger` | A signed integer type was required. |
| `ExpectedUnsignedInteger` | An unsigned integer type was required. |
| `ExpectedArray` | An array type was required. |
| `ExpectedString` | A string type was required. |
| `ExpectedArrayOrString` | An array-like type was required. |
| `ExpectedStruct` | A struct type was required. |
| `ExpectedPointer` | A runtime pointer type was required. |
| `ExpectedFunctionType` | A function signature type was required. |
| `ExpectedFunctionPointer` | A function-pointer type was required. |
| `AssignmentTypeMismatch` | A value cannot be assigned to the destination type. |
| `UnexpectedType` | A value/literal has a different exact type than required. |

##### Aggregates, fields, and indexing

| Code | Meaning |
|---|---|
| `FieldIndexOutOfBounds` | A struct field index is outside the valid range. |
| `FieldCountMismatch` | A struct literal has the wrong number of initialized fields. |
| `MissingField` | A required struct-literal field was not initialized. |
| `MultipleInitializationsOfSameField` | A struct-literal builder initializes one field more than once. |
| `FieldNotFound` | A field name does not exist in the struct type. |
| `IndexOutOfBounds` | A compile-time array/string index is outside the valid range. |
| `NegativeIndex` | An index literal is negative. |
| `TooManyElementsInArrayInitialization` | An array-literal builder received more elements than its type permits. |
| `TooFewElementsInArrayInitialization` | An array-literal builder was finalized before all elements were supplied. |

##### Calls, input, and builders

| Code | Meaning |
|---|---|
| `FunctionParameterCountMismatch` | A function type or call has the wrong number of parameters/arguments. |
| `InvalidFunctionPointerCall` | An indirect call has an invalid argument count or argument types. |
| `InvalidReadType` | `read()` was given a destination whose storage size is not one slot. |
| `BuilderNotFinalized` | A builder was destroyed without its required `begin()` or `done()`. |
| `BuilderUsedAfterFinalize` | A finalized builder was used again. |
| `ReturnTypeSpecifiedMultipleTimes` | `ret()` was called more than once on a function or function-type builder. |

</details>


## Building

Acus has no external dependencies beyond a C++23 compiler, GNU Make, and the C++ standard library.

```sh
git clone https://github.com/jorenheit/acus.git
cd acus
make
make check
```

The static library is written to `lib/libacus.a`.

To build the included examples:

```sh
make examples
```

Installation is optional:

```sh
sudo make install
```

The default prefix is `/usr/local`. A custom prefix can be selected with, for example:

```sh
make install PREFIX="$HOME/.local"
```

## Project status

Acus is under active development. The backend is already capable of producing substantial programs, but the public API and internal representations may still evolve.

Current design constraints include:

- 8- and 16-bit integer types only;
- fixed-capacity arrays and strings;
- potentially large and slow generated Brainfuck programs;
- a single-threaded program-construction model;
- no promise of stable API compatibility between early versions.

These are not all permanent limitations, but they are useful expectations when building on the project today.

## Related interfaces

### Acus Sugar

[Acus Sugar](include/acus/sugar/README.md) is a C++23 embedded language layered on top of Acus. It provides operator syntax, variables, functions, loops, conditionals, arrays, structs, pointers, and a small standard library. It is the better starting point for writing programs directly.

The core Acus API remains focused on backend construction and on giving frontends explicit control over generated program structure.

## Contributing

Bug reports, focused tests, documentation improvements, new backend algorithms, and experimental frontends are welcome.

Particularly useful reports include:

- a minimal Acus program that reproduces the problem;
- the generated Brainfuck or primitive sequence when relevant;
- expected and actual runtime output;
- measurements showing changes in generated size or instruction count.

If you build a language or unusual tool on top of Acus, please share it. Exploring those frontends is one of the main reasons the project exists.

## License

Acus is licensed under the GNU General Public License v3.0 or later. See [LICENSE](LICENSE) for the full license text.
