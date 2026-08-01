# `bfint`

`bfint` is the Brainfuck interpreter bundled with Acus. It can run programs
from a file or from standard input, supports an optional random-value
extension, provides an ncurses-based gaming mode for non-blocking keyboard
input and screen output, and can write detailed execution profiles—even when a
program is interrupted with `SIGINT`.

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

Run a Brainfuck program from a file:

```sh
bfint program.bf
```

When no file is supplied, the Brainfuck source is read from standard input:

```sh
bfint < program.bf
```

or:

```sh
cat program.bf | bfint
```

> [!NOTE]
> When the program itself is read from standard input, that stream is consumed
> before execution begins. A Brainfuck program containing `,` therefore cannot
> use the same stream for runtime input. Supply such programs through a file
> instead.

### Options

```text
-h, --help             Show command-line help.
-p, --profile FILE     Write execution statistics to FILE.
--gaming               Enable ncurses gaming mode.
--gaming-help          Explain gaming-mode terminal handling.
--random               Enable the `?` random-value extension.
--rand-max N           Set the maximum generated random value.
--no-random-warning    Suppress warnings for `?` without --random.
```

## Profiling

Use `-p` or `--profile` to write execution statistics to a file:

```sh
bfint --profile profile.txt program.bf
```

The profile is written after normal completion, after a runtime error, and when
the program is interrupted with `Ctrl+C` (`SIGINT`). An interrupted run exits
with status code 130 after restoring the terminal and writing the profile.

The report includes:

- whether execution completed, failed, or was interrupted;
- elapsed execution time;
- source instruction and preprocessed-operation counts;
- total executed instructions and execution rate;
- the highest tape address and number of cells touched;
- maximum active loop depth;
- per-instruction execution counts and percentages.

For a program read from standard input, the profile identifies the source as
`<stdin>`.

## Random-value extension

With `--random`, the non-standard `?` instruction stores a random value in the
current cell:

```sh
bfint --random program.bf
```

Values range from zero through 255 by default. Set a different inclusive upper
bound with `--rand-max`:

```sh
bfint --random --rand-max 9 program.bf
```

Without `--random`, `?` does not modify the current cell. By default, `bfint`
prints a warning the first time it encounters one; use
`--no-random-warning` to suppress it.

## Gaming mode

Gaming mode replaces normal terminal input and output with ncurses:

```sh
bfint --gaming game.bf
```

Input becomes non-blocking. Whenever a Brainfuck `,` instruction is executed,
a pending keypress is stored in the current cell; when no key is available,
zero is stored instead. This makes it possible to run games and other
interactive programs without waiting for Enter after every keypress.

Gaming mode also translates a supported subset of ANSI cursor-positioning and
screen-clearing escape sequences into ncurses operations. Run:

```sh
bfint --gaming-help
```

for the complete list of supported sequences.
