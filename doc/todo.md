# Acus Sugar Layer TODO

A working checklist for extending the Acus syntactic sugar layer into a small, human-friendly programming interface.

## Priority 1 — Foundation

### Error handling and source locations

- [ ] Add a sugar-level error wrapper around API calls.
- [ ] Capture the user call site with `std::source_location::current()`.
- [ ] Rewrite or wrap `acus::error::Error` so diagnostics point to the DSL source line, not the sugar implementation.
- [ ] Apply this consistently to:
  - [ ] `program`
  - [ ] `function`
  - [ ] `function_fwd`
  - [ ] `let`
  - [ ] `var`
  - [ ] `return_`
  - [ ] `if_`
  - [ ] `while_`
  - [ ] `for_`
  - [ ] `print` / `println`
  - [ ] function calls
  - [ ] struct and array access

Possible implementation shape:

```cpp
template <typename F>
decltype(auto) sugarCall(std::source_location loc, F&& f) {
  try {
    return std::forward<F>(f)();
  } catch (acus::error::Error& err) {
    err.setLocation(loc.file_name(), loc.line(), loc.column());
    throw;
  }
}
```

For macro-based syntax, pass the source location explicitly from the macro.

---

### Scope handling

- [ ] Add a sugar-level RAII scope guard.
- [ ] Make `if_`, `while_`, `for_`, and explicit `scope_` use this guard.
- [ ] Confirm that locals declared inside a block are freed safely.
- [ ] Decide whether the sugar layer should mangle local names per scope.
- [ ] Add regression tests for:
  - [ ] local declared inside `if_`
  - [ ] local declared inside `while_`
  - [ ] local declared inside `for_`
  - [ ] nested locals with the same source name

Possible syntax:

```cpp
scope_({
  let<u8>("tmp") = 42;
  println(var("tmp"));
})
```

---

## Priority 2 — Input and output

### Raw input

- [ ] Add raw byte input based on the BF comma command.
- [ ] Decide whether raw input should be expression-style or assignment-style.

Possible syntax:

```cpp
read(var("c"));
let<u8>("c") = readChar();
```

Recommended naming:

```cpp
read(var("x"));       // raw byte into existing variable
readChar();           // returns one character / byte
```

---

### Decimal input

- [ ] Add unsigned decimal parsing.
- [ ] Add signed decimal parsing.
- [ ] Decide how invalid input is handled.
- [ ] Decide whether newline is consumed.
- [ ] Add tests for `u8`, `s8`, `u16`, and `s16`.

Possible syntax:

```cpp
let<u8>("n") = readDec<u8>();
let<s8>("x") = readDec<s8>();

readDec(var("n"));
```

---

### String input

- [ ] Add fixed-size string input.
- [ ] Decide whether strings are newline-terminated, zero-terminated, or fixed-count.
- [ ] Decide what happens on overflow.
- [ ] Add `readLine<N>()` or `readString<N>()`.

Possible syntax:

```cpp
let<string<20>>("name") = readLine<20>();
readLine(var("name"));
```

---

### Output distinction

- [ ] Keep or clarify the distinction between raw output and formatted output.
- [ ] Add explicit wrappers for:
  - [ ] `write(expr)` — raw output
  - [ ] `print(expr)` — formatted output
  - [ ] `println(expr)` — formatted output + newline
  - [ ] `printChar(expr)`
  - [ ] `printDec(expr)`
  - [ ] `printString(expr)`

---

## Priority 3 — Operators and expressions

### Assignment operators

- [ ] Add or verify:
  - [ ] `+=`
  - [ ] `-=`
  - [ ] `*=`
  - [ ] `/=`
  - [ ] `%=`

### Unary operators

- [ ] Add unary logical not: `!expr`
- [ ] Add unary minus: `-expr`
- [ ] Add `abs(expr)`.
- [ ] Add `bool_(expr)` or equivalent normalization to `0` / `1`.

### Logical and comparison operators

- [ ] Verify:
  - [ ] `==`
  - [ ] `!=`
  - [ ] `<`
  - [ ] `<=`
  - [ ] `>`
  - [ ] `>=`
  - [ ] `&&`
  - [ ] `||`

### Optional named logic helpers

Add these only if they map cleanly to the Acus model:

- [ ] `xor_(a, b)`
- [ ] `xnor_(a, b)`
- [ ] `nand_(a, b)`
- [ ] `nor_(a, b)`

---

## Priority 4 — Control flow

### Existing structured control flow

- [ ] Stabilize `if_`.
- [ ] Stabilize `while_`.
- [ ] Stabilize `for_`.
- [ ] Confirm all bodies are scoped correctly.
- [ ] Confirm source locations are correct for errors inside bodies.

### Additional structured control flow

- [ ] Add `doWhile_`.
- [ ] Add `repeat_(count, body)`.
- [ ] Add `loop_(body)` for infinite loops.

Possible syntax:

```cpp
repeat_(10, {
  println("hello");
})
```

### Loop control

- [ ] Add `break_()`.
- [ ] Add `continue_()`.
- [ ] Maintain a sugar-level stack of active loop targets.
- [ ] Produce a clear error when `break_` or `continue_` is used outside a loop.

### Assertions and low-level control

- [ ] Add `assert_(condition)`.
- [ ] Add `abort_()`.
- [ ] Add `unreachable_()`.

---

## Priority 5 — Functions

### Function declarations and calls

- [ ] Keep improving `function_fwd`.
- [ ] Support definition from function handle.

Possible syntax:

```cpp
auto fib = function_fwd<void(u8, u8, u8)>("fib");

function(fib, "n", "a", "b");
{
  // ...
}
endFunction();
```

### Return values

- [ ] Stabilize `return_()`.
- [ ] Stabilize `return_(expr)`.
- [ ] Confirm nested return expressions work.
- [ ] Confirm recursive function calls work.
- [ ] Confirm return values interact correctly with the cache/materialization system.

### Ignoring return values

- [ ] Decide whether non-void return values may be ignored.
- [ ] If needed, add `discard(expr)`.

---

## Priority 6 — Types, structs, and arrays

### Structs

Current target style:

```cpp
using Point = Struct<"Point",
  Field<"x", u8>,
  Field<"y", u8>
>;
```

- [ ] Keep type-level struct definitions.
- [ ] Support value initialization:

```cpp
let<Point>("p") = Point{'X', 'Y'};
```

- [ ] Add typed field access if possible:

```cpp
var("p").field<"x">()
```

- [ ] Keep fallback string field access:

```cpp
var("p").field("x")
```

- [ ] Add clearer errors for unknown fields.
- [ ] Add clearer errors for wrong initializer arity.

### Arrays

Current target style:

```cpp
using Vec2 = Array<u8, 2>;
let<Vec2>("v") = Vec2{'X', 'Y'};
```

- [ ] Support value initialization.
- [ ] Support static index access.
- [ ] Support dynamic index access.
- [ ] Add `fill(var("arr"), value)`.
- [ ] Add `copy(var("dst"), var("src"))`.
- [ ] Add `length<T>()` or equivalent compile-time size helper.
- [ ] Add `printArray`.
- [ ] Add `printlnArray`.

### Strings

- [ ] Clarify fixed-size string semantics.
- [ ] Add `clear(str)`.
- [ ] Add `copy(dst, src)`.
- [ ] Add `equals(a, b)`.
- [ ] Add `length(str)` if feasible.
- [ ] Add `appendChar(str, c)` if feasible.

---

## Priority 7 — Globals, pointers, and low-level escape hatches

### Globals

The assembler exposes global declarations, so add sugar for them.

Possible syntax:

```cpp
global<u8>("counter");
global<string<20>>("message");
```

- [ ] Add `global<T>("name")`.
- [ ] Decide whether global initialization is supported.
- [ ] Enforce that globals are declared only where the API permits.
- [ ] Add readable errors for invalid global declarations.

### Pointers

- [ ] Add a sugar pointer type, for example `ptr<T>`.
- [ ] Add `addr(var("x"))`.
- [ ] Add `deref(var("p"))`.
- [ ] Support assignment through dereferenced pointers.
- [ ] Add pointer examples and tests.

Possible syntax:

```cpp
let<ptr<u8>>("p") = addr(var("x"));
deref(var("p")) = 12;
```

### Labels and jumps

Expose low-level control as an escape hatch, probably under a `raw` namespace.

- [ ] `raw::label("name")`
- [ ] `raw::jump("name")`
- [ ] `raw::jumpIf(condition, "then", "else")`

---

## Priority 8 — Random numbers

Acus has a non-standard BF random operation. Expose it in the sugar layer.

- [ ] Add `rand<u8>()`.
- [ ] Add `randRange(upper)`.
- [ ] Add `randRange(low, high)`.
- [ ] Decide whether the upper bound is inclusive or exclusive.
- [ ] Document that this depends on non-standard BF backend support.
- [ ] Add an example program such as dice rolling or number guessing.

Possible syntax:

```cpp
let<u8>("x") = rand<u8>();
let<u8>("die") = randRange<u8>(1, 6);
```

---

## Priority 9 — Small standard library

Keep this separate from the core sugar layer, for example:

```cpp
#include "acus/sugar/std.h"
```

### Math helpers

- [ ] `min(a, b)`
- [ ] `max(a, b)`
- [ ] `clamp(x, lo, hi)`
- [ ] `abs(x)`
- [ ] `sign(x)`
- [ ] `square(x)`
- [ ] `pow(base, exp)` for small integer exponents
- [ ] `isEven(x)`
- [ ] `isOdd(x)`
- [ ] `gcd(a, b)`

### Division helpers

- [ ] Add `divmod(a, b)`.
- [ ] Consider returning a small struct:

```cpp
using DivMod = Struct<"DivMod",
  Field<"quot", u8>,
  Field<"rem", u8>
>;
```

### IO helpers

- [ ] `prompt("N = ", var("n"))`
- [ ] `printSep(a, b, sep)`
- [ ] `printArray(arr)`
- [ ] `printlnArray(arr)`
- [ ] `readDec<T>()`
- [ ] `readLine<N>()`

### String helpers

- [ ] `clear(str)`
- [ ] `copy(dst, src)`
- [ ] `equals(a, b)`
- [ ] `length(str)`
- [ ] `appendChar(str, c)`

---

## Priority 10 — Examples and regression tests

Add small programs that double as feature tests.

- [ ] `hello_world`
- [ ] `fibonacci_iterative`
- [ ] `fibonacci_recursive_generator`
- [ ] `read_and_echo`
- [ ] `decimal_input`
- [ ] `guess_the_number`
- [ ] `array_sum`
- [ ] `struct_point`
- [ ] `gcd`
- [ ] `global_counter`
- [ ] `pointer_demo`
- [ ] `random_dice`
- [ ] `nested_scopes`
- [ ] `break_continue`

Each example should test one or two features clearly, not everything at once.

---

## Suggested implementation order

1. [ ] Error handling and source locations.
2. [ ] RAII scopes and block-local variable tests.
3. [ ] Raw input and raw output.
4. [ ] Decimal input.
5. [ ] String input.
6. [ ] Complete assignment and unary operators.
7. [ ] `break_` and `continue_`.
8. [ ] Random number sugar.
9. [ ] Globals.
10. [ ] Pointer sugar.
11. [ ] Small `std` library.
12. [ ] More examples and regression tests.

---

## Notes

- Locals declared inside loop and conditional bodies should be valid.
- Acus declarations happen at code-generation time, not runtime.
- Sugar scopes should therefore model lexical scope and safely map names to Acus slots.
- The API-level frame can still remain function-local; the sugar layer can enforce higher-level scoping rules.
- Keep the core sugar small. Put convenience algorithms and IO helpers in a separate standard library header.
