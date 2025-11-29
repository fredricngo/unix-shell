# unix-shell

A simple, working Unix shell written in C.

## Overview

This project implements a basic Unix shell supporting:

- Built-in commands: `exit`, `cd`, `pwd`
- Execution of external commands
- Command parsing with multiple semicolon-separated commands
- Output redirection with `>` and advanced redirection using `>+`
- Batch mode: executes commands from a file

## Compilation

Compile the shell using GCC:

```sh
gcc -o myshell myshell.c
```

## Usage

### Interactive Mode

Launch the shell interactively:

```sh
./myshell
```

The prompt appears as:

```
myshell>
```

Type commands just as you would in a regular shell.

### Batch Mode

Run commands from a batch file:

```sh
./myshell batchfile.txt
```

### Features

- **Built-in commands**:  
  - `exit` — Exits the shell.
  - `cd [directory]` — Change directory. No argument changes to `$HOME`.
  - `pwd` — Print current directory.

- **External commands**:  
  Any program that exists in your system's PATH.

- **Redirection**:  
  - `>` — Redirect output to a file (overwrites).
  - `>+` — Advanced redirection that preserves old file content after command output.

- **Error handling**:  
  Prints a standard error message for any failure.

## Notes

- Written for educational purposes.
- Requires a Unix-like OS (Linux, macOS).
- No support for piping (`|`), background execution (`&`), or environment variable expansion.
- Uses only standard C libraries.

## License

MIT License
