# micro-sh

A lightweight POSIX-compliant command-line interpreter implemented in C.

## Features

- **Interactive & Batch Mode:** Run commands interactively from the shell prompt or pass a script file as a command-line argument.
- **Built-in Commands:**
  - `exit`: Terminate the shell.
  - `cd [dir]`: Change directories (defaults to `$HOME` if no path is provided).
- **Process Management:**
  - Standard synchronous foreground process execution using `fork()` and `execvp()`.
  - Asynchronous background jobs via a trailing `&`.
  - Signal-safe zombie child reaping using `SIGCHLD`, `sigaction` (`SA_RESTART`), and `waitpid()`.
- **Parsing:** Handles inline comments (`#`), strips whitespace, and enforces argument bounds.
- **I/O Redirection:** Handles redirection of STDIN and STDOUT(`<`, `>`)
- **Pipeline chaining:** Supports piping of multiple (up to 16) commands (`|`)
- Terminal job control & forward `SIGINT` (Ctrl+C) handling

## Getting Started

### Prerequisites

- GCC or Clang
- A POSIX-compatible environment (Linux, macOS, WSL)

### Compilation

Compile with standard warnings and flags:

```bash
gcc -Wall -Wextra -pedantic -std=c99 -o myshell src/myshell.c
```

### Usage

- **Interactive shell:**

  ```bash
  ./myshell
  ```

- **Batch / Script mode:**

  ```bash
  ./myshell script.sh
