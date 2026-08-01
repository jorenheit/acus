# Acus Sugar examples

Each `.acs` file is a complete input for `acs`. The examples begin with small
language features and progress to larger programs.

First build Acus from the repository root:

```sh
cmake -S . -B build
cmake --build build
```

Compile one example directly from the checkout:

```sh
./tools/acs/acs.py tools/acs/examples/01_hello.acs \
  --acus-root . \
  -o hello.bf
```

With an installed frontend and library:

```sh
acs tools/acs/examples/01_hello.acs -o hello.bf
```

For a user-local installation, add `--acus-prefix "$HOME/.local"`.

Compile every example from the repository root with:

```sh
for source in tools/acs/examples/*.acs; do
  ./tools/acs/acs.py "$source" --acus-root . -o "${source%.acs}.bf"
done
```

## Contents

- `01_hello.acs` — the smallest complete program.
- `02_variables_and_arithmetic.acs` — integer types, expressions, casts, and
  compound assignment.
- `03_control_flow.acs` — `if_`, `else_`, `for_`, `while_`, `break_`, and
  `continue_`.
- `04_functions.acs` — declarations, parameters, return values, and calls.
- `05_recursion.acs` — a recursive Fibonacci function.
- `06_arrays_and_structs.acs` — aggregate types, fields, and dynamic indexing.
- `07_pointers.acs` — address-of, dereferencing, pointer parameters, and pointer
  arithmetic.
- `08_globals.acs` — global storage shared by functions.
- `09_input_and_strings.acs` — standard-library line input and string length.
- `10_prime_sieve.acs` — prime-number generation.
- `11_mandelbrot.acs` — terminal-rendered Mandelbrot output.
- `11_mandelbrot_no_screen.acs` — Mandelbrot output without screen control.
- `12_pong.acs` — a larger interactive demonstration.
