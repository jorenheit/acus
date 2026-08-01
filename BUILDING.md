# Building Acus

Acus requires CMake 3.21 or newer and a C++23 compiler.

Optional components have additional requirements:

- `acs` requires Python 3 when it is run;
- `bfint` requires the ncurses development files;
- the checked-in tests currently use POSIX process APIs and are intended for
  Unix-like systems.

## Basic build

Configure and build the static library from the repository root:

```sh
git clone https://github.com/jorenheit/acus.git
cd acus
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The library is written to `build/lib/libacus.a`.

## Optional components

All options are set while configuring the root project:

```sh
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Debug \
  -DACUS_BUILD_EXAMPLES=ON \
  -DACUS_BUILD_TESTS=ON \
  -DACUS_BUILD_BFINT=ON \
  -DACUS_INSTALL_ACS=ON
cmake --build build
```

| Option | Default | Effect |
|---|---:|---|
| `ACUS_BUILD_EXAMPLES` | `OFF` | Build the C++ examples in `examples/`. |
| `ACUS_BUILD_TESTS` | `OFF` | Build and register the runtime and compile-time test suites. |
| `ACUS_BUILD_BFINT` | `OFF` | Build and install the bundled Brainfuck interpreter. |
| `ACUS_INSTALL_ACS` | `ON` | Include the `acs` Python frontend in installation. |
| `ACUS_ENABLE_WARNINGS` | `ON` | Enable the project warning flags. |

Run the tests with:

```sh
ctest --test-dir build --output-on-failure
```

Use `--verbose` to display test output while a test is running.

>[!WARNING]
>The testsuite contains tests that take a very long time to execute.

## Presets

The repository includes three convenience presets:

```sh
cmake --preset default
cmake --build --preset default

cmake --preset dev
cmake --build --preset dev
ctest --preset dev

cmake --preset release
cmake --build --preset release
```

`default` builds only the library. `dev` also enables examples, tests and
`bfint`; it therefore requires ncurses. `release` builds an optimized library. Enable `bfint` explicitly when it is
needed.

## Installation

Install the components enabled during configuration:

```sh
cmake --install build
```

The default prefix is normally `/usr/local`. For a user-local installation:

```sh
cmake --install build --prefix "$HOME/.local"
```

This installs the static library, public headers, `acs`, and—when enabled—
`bfint`.

Remove files recorded by the most recent installation from this build tree:

```sh
cmake --build build --target uninstall
```

Use the same build directory that was used for installation.

## Building a single example

After configuring with `ACUS_BUILD_EXAMPLES=ON`, build a named example target:

```sh
cmake --build build --target acus-example-fib_iterative
```

The helper script configures a suitable build directory automatically:

```sh
examples/compile.sh fib_iterative.cc
```

Set `ACUS_BUILD_DIR` to make the helper use a different build directory.
