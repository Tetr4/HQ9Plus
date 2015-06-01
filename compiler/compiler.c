#include <stdio.h>
#include <stdlib.h>

/*
!!! Work in Progress !!!

Compiler for HQ9+ files (http://esolangs.org/wiki/HQ9)
Outputs x86_64 assembly in AT&T syntax.

H: Print "hello, world"
Q: Print the program's source code
9: Print the lyrics to "99 Bottles of Beer"
+: Increment the accumulator

Compile the compiler:
    gcc -ansi -pedantic -Wall compiler.c -o HQ9+

Compile and assemble a HQ9+ program:
    long:
        ./HQ9+ ../main.hq9+ > generated_assembly.s
        gcc -nostartfiles -o program generated_assembly.s
    
    short:
        ./HQ9+ ../main.hq9+ | gcc -nostartfiles -o program -xassembler -
    
Start the program:
    ./program
    
Fast testing:
    gcc -ansi -pedantic -Wall compiler.c -o HQ9+ && ./HQ9+ ../main.hq9+ | gcc -nostartfiles -o program -xassembler - && ./program
*/

void print_hello_world();
void print_source_code(char* filename);
void print_bottles_of_beer(int bottle_count);
void increment_the_accumulator(int* accumulator);

int main(int argc, char **argv)
{
    FILE *file;
    char instruction;
    char* filename;
    
    if(argc != 2)
    {
        /* wrong number of args */
        fprintf(stderr, "Error: exactly 1 HQ9+ source file as arg required\n");
        exit(EXIT_FAILURE);
    }
    
    /* open file */
    filename = argv[1];
    file = fopen(filename, "r");
    if (file == NULL)
    {
        /* could not open file */
        perror("Error opening file");
        exit(EXIT_FAILURE); 
    }
    
    /* data segment */
    puts(
      ".data\n"
      "hello:\n"
      "  .asciz \"hello world \\n\"\n"
      "bottles:\n"
      "  .asciz \"%d bottles of beer\\n\"\n" /* TODO whole song */
      "accumulator:\n"
      "  .long 0\n"
    );

    /* begin text segment */
    puts(
      ".text\n"
      ".globl _start\n"
      "_start:\n"
      /* align stack pointer (%rsp) with multiple of 16 Byte. 
         Stack starts at high address and grows down -> subtract
         suffix: b - Byte / 8  bit
                 w - Word / 16 bit
                 l - Long / 32 bit
                 q - Quad / 64 bit <- x86_64 */
      "  subq $8,  %rsp\n"
      /* In a function we would align the stack pointer by
         saving the base (or frame) pointer (%rbp) on the stack:
         push %rbp
         movq %rsp, %rbp
         # your code here...
         popq $rbp */
    );

    
    /* parse file */
    while((instruction = fgetc(file)) != EOF)
    {
        switch (instruction)
        {
            case 'H':
                /* call printf("hello world %d\n", 42) */
                puts(
                  /* don't use printf varargs -> store 0 as vararg length */
                  "  movb $0, %al\n"
                  /* load effective address of string relativ to
                     instruction pointer (%rip) as first argument (%rdi).
                     Argument sequence for function calls:
                     %rdi, %rsi, %rdx, %rcx, %r8, %r9 then on the stack in reverse order */
                  "  leaq hello(%rip), %rdi\n"
                  "  call printf\n"
                );
                break;
                
            case 'Q':
                /* TODO print the program's source code */
                break;
                
            case '9':
                puts(
                  "  movb $0, %al\n"
                  "  leaq bottles(%rip), %rdi\n"
                  /* accumulator as second argument for now*/
                  "  movq accumulator(%rip), %rsi\n"
                  "  call printf\n"
                );
                break;
                
            case '+':
                puts("incq accumulator(%rip) \n");
                break;
        }
    }
    if (!feof(file)) {
        perror("Error reading file");
    }
    fclose(file);
    
    /* call exit(0) */
    puts(
      "  movq $0, %rdi\n"
      "  call _exit\n"
    );
    
    exit(EXIT_SUCCESS);
}


