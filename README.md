# HQ9+ Interpreter, Compiler and JIT-Compiler
See http://esolangs.org/wiki/HQ9 for more on HQ9+. <br>
See [Interpreter, Compiler, JIT](https://nickdesaulniers.github.io/blog/2015/05/25/interpreter-compiler-jit/) for more on translation techniques.

## Instructions
+ H: Print "hello, world"
+ Q: Print the program's source code
+ 9: Print the lyrics to "99 Bottles of Beer"
+ +: Increment the accumulator

## Interpreter
+ Compile the interpreter: `gcc -ansi -pedantic -Wall interpreter.c -o HQ9+`
+ Interpret a HQ9+ program: `./HQ9+ ../main.hq9+`

## Compiler
+ Compile the compiler: `gcc -ansi -pedantic -Wall compiler.c -o HQ9+`
+ Compile and assemble a HQ9+ program: `./HQ9+ ../main.hq9+ | gcc -no-pie -nostartfiles -o program -xassembler -`
+ Start the program: `./program`

## JIT-Compiler
+ Compile the jit compiler: `gcc jit.c vector.c -o HQ9+`
+ Jit a program: `./HQ9+ ../main.hq9+`
