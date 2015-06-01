#include <stdio.h>
#include <stdlib.h>

/*
!!! Work in Progress !!!

Compiler for HQ9+ files (http://esolangs.org/wiki/HQ9)

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
        ./HQ9+ ../main.hq9+ | gcc -nostartfiles -o program -xassembler
    
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
    
    char* prologue;
    char* epilogue;
    char* bottles;
    char* hello_world;
        
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
    
    prologue =
      ".data\n"
      "hello:\n"
      "  .asciz \"hello world \\n\"\n"
      "bottles:\n"
      "  .asciz \"%d bottles of beer\\n\"\n" /* TODO whole song */
      "accumulator:\n"
      "  .long 0\n"
      ".text\n"
      ".globl _start\n"
      "_start:\n"
      "  subq $8,  %rsp\n";
      
    epilogue =
      "  movq $42, %rdi\n"
      "  call _exit\n";
    
    bottles = 
      "  movb $0, %al\n"
      "  leaq bottles(%rip), %rdi\n"
      "  movq accumulator(%rip), %rsi\n"
      "  call printf\n";
      
    hello_world = 
      "  movb $0, %al\n"
      "  leaq hello(%rip), %rdi\n"
      "  call printf\n";  
      
    puts(prologue);
    
    /* parse file */
    while((instruction = fgetc(file)) != EOF)
    {
        switch (instruction)
        {
            case 'H':
                puts(hello_world);
                break;
            case 'Q':
                /* TODO print the program's source code */
                break;
            case '9':
                puts(bottles);
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
    
    puts(epilogue);
    
    exit(EXIT_SUCCESS);
}


