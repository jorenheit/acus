# `bfint`

`bfint` is the Brainfuck interpreter bundled with Acus. It supports ordinary
Brainfuck programs, a configurable tape size, an optional random-value
extension, and an ncurses-based gaming mode for non-blocking keyboard input and
screen output.

## Requirements

`bfint` requires:

- CMake 3.21 or newer;
- a C++23 compiler;
- the ncurses development files.

On Debian or Ubuntu, ncurses can be installed with:

```sh
sudo apt install libncurses-dev
```

## Building

Enable `bfint` while configuring the Acus repository:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DACUS_BUILD_BFINT=ON
cmake --build build --target bfint
```

The executable is written to `build/bin/bfint`.

Install it together with the enabled Acus components:

```sh
sudo cmake --install build
```

For a user-local installation:

```sh
cmake --install build --prefix "$HOME/.local"
```

## Usage

Run a Brainfuck program with:

```sh
bfint program.bf
```

Common options are:

```text
-h, --help          show command-line help
-n N                use a tape containing N cells
--gaming            enable ncurses gaming mode
--gaming-help       explain gaming-mode terminal handling
--random            enable the `?` random-value extension
--rand-max N        set the maximum generated random value
--no-random-warning suppress warnings for `?` without --random
```

Gaming mode uses non-blocking input: when a Brainfuck `,` command is executed
without a pending keypress, zero is stored in the current cell. It also
translates the supported ANSI cursor and clear-screen sequences to ncurses
operations. Run `bfint --gaming-help` for the full list.
