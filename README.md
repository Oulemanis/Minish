# minish

`minish` is a small Unix-like shell written in C for an undergraduate Operating Systems course.
Based on [OS-AIC303/minish-base](https://github.com/OS-AIC303/minish-base).

## Repository layout

```text
minsh-base/
├── include/
│   └── parser.h          # Parser interface supplied with the project
├── src/
│   ├── parser.c          # Lexer and parser supplied with the project
│   └── shell.c           # Main student implementation
├── Makefile
├── README.md
├── SUBMIT.md             # Student names, implementation description
└── assignment-1.pdf
```

The parser is provided so students can focus on process execution rather than
writing a lexer and parser. The main implementation task is to complete
`execute_pipeline()` in `src/shell.c` and add helper functions as needed.

## Requirements

- Linux or another POSIX-compatible environment.
- A C compiler such as GCC or Clang.
- `make`.
- A POSIX shell to run the test script.

The project uses strict compiler warnings. The default build flags include
`-std=c11 -Wall -Wextra -Wpedantic -Werror`.

## Build

From the repository root:

```sh
make
```

The executable is created at:

```text
build/minish
```

To remove generated build files:

```sh
make clean
```

## Run

Start the shell interactively:

```sh
./build/minish
```

You can also send commands to it through standard input, which is useful for
testing:

```sh
printf 'echo hello\nexit\n' | ./build/minish
```

The prompt is displayed when standard input is connected to a terminal.
