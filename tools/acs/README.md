# `acs`

`acs` compiles an Acus Sugar source file to Brainfuck. It generates a temporary
C++ translation unit, compiles and links it against `libacus`, and runs the
resulting generator. Brainfuck is written to standard output unless `-o` is
used.

## Requirements

`acs` requires Python 3, a C++23 compiler, the Acus headers, and `libacus`.

## Source format

An `.acs` file contains the contents of an Acus program scope. It must define
the entry function `main` and may define any number of helper functions:

```cpp
function_<void()>("main") | define {
  println("Hello, World!");
  return_;
};
```

## Building and installation

From the repository root, build and install Acus together with the frontend:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DACUS_INSTALL_ACS=ON
cmake --build build
sudo cmake --install build
```

For a user-local installation:

```sh
cmake --install build --prefix "$HOME/.local"
```

Ensure `$HOME/.local/bin` is on `PATH` when using that prefix.

## Usage

With a system installation:

```sh
acs hello.acs > hello.bf
acs hello.acs -o hello.bf
```

For a custom installation prefix:

```sh
acs hello.acs --acus-prefix "$HOME/.local" -o hello.bf
```

When using `acs.py` directly from an Acus checkout, point it at the source
root. It will find libraries produced in the standard CMake build directories:

```sh
cmake -S . -B build
cmake --build build
./tools/acs/acs.py tools/acs/examples/01_hello.acs --acus-root . -o hello.bf
```

The equivalent environment variables are `ACUS_ROOT` and `ACUS_PREFIX`.
Other useful options include:

```text
--compiler clang++       choose the C++ compiler
-I DIR                   add an include directory
-L DIR                   add a library directory
--acus-lib FILE          link an exact library file
--cxxflag FLAG           pass an extra compiler flag
--ldflag FLAG            pass an extra linker flag
--emit-cpp FILE          keep a copy of the generated wrapper
--keep-build             retain temporary build files
-v                       print executed commands
```

Run `acs --help` for the complete command-line reference.

## Examples

The [`examples`](examples/) directory contains small `.acs` programs covering
variables, control flow, functions, recursion, aggregates, pointers, globals,
input, strings, and larger demonstrations.
