# Bobcat

Bobcat is a simple, statically typed, low level and unfinished language.

The goal of this language is to be fast to write and execute. It can both be interpreted for fast development & debugging, and transpiled to C for maximum performances.
The folder `samples` contains a few Bobcat programs, showing it's features. They are also test files, and contains edge cases to test the parser.

```
func main() { 
        prints("Hello World!\n")
}
```

The interpreter/transpiler is writted from scratch in C. It can show the compiling trees at different stages (tokenizer -> parser -> semantical).

The `jit` branch is using LLVM for compilation & JIT execution, but is not finished currently.

## Todo list
```
[x] Function overloading
[x] Interpreter semfriendly
[x] Assignation at declaration
[x] Function return value
[x] Valgrind & linting
[x] Break & continue
[x] Unify mains
[x] Scalable tokenizer
[x] Memory leaks
[*] LLVM
 - [x] Functions def&call
 - [x] Operators
 - [x] Assignations
 - [x] Flow control
 - [x] Top level instructions
 - [-] Compilation
[-] Read–eval–print loop
[-] Multiple files
[-] Structs
[-] Cast
[-] Operators overloading
[-] Pointers / Arrays
[-] Preprocessor
[-] Stdlib
```
