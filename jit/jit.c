#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include "vector.h"

/*
!!! Work in Progress !!!
TODO: no errors/warnings on 'gcc -ansi -pedantic -Wall jit.c vector.c -o jit'
TODO: functionality for H,Q and 9 instructions

JIT-Compiler for HQ9+ files (http://esolangs.org/wiki/HQ9)

H: Print "hello, world"
Q: Print the program's source code
9: Print the lyrics to "99 Bottles of Beer"
+: Increment the accumulator

Compile the jit compiler:
    gcc jit.c vector.c -o jit

Jit a program:
    ./jit ../main.hq9+

Debug output:
    gcc jit.c vector.c -o jit && ./jit ../main.hq9+ | hexdump -C

Test assembly:
    gcc -nostartfiles -o assembly_test assembly.s && objdump -s assembly_test
*/

/* libc function as argument, for easy access */
typedef int fn_putchar (int);
int write_to_stack(struct vector* const vec, char* text);

int main(int argc, char **argv)
{
    FILE *file;
    char instruction;
    char* filename;
    struct vector instruction_stream;

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


    /*** prologue ***/
    vector_create(&instruction_stream, 100);
    char prologue [] = {
        0x55, // push %rbp
        0x48, 0x89, 0xE5, // mov %rsp, %rbp

        // backup %r12 (callee saved register)
        0x41, 0x54, // pushq %r12
        // store %rdi content (putchar) in %r12 as callee saved
        0x49, 0x89, 0xFC, // movq %rdi, %r12

        // push accumulator on stack
        // accumulator address: -0x10(%rbp)
        0x6a, 0x00, // pushq $0
    };
    vector_push(&instruction_stream, prologue, sizeof(prologue));
    int rsp = -0x10;

    int text_bytes_on_stack = 0l;

    text_bytes_on_stack += write_to_stack(&instruction_stream, "Hello World");
    int offset_hello_world = -text_bytes_on_stack + rsp;

    text_bytes_on_stack += write_to_stack(&instruction_stream, "Q");
    int offset_source = -text_bytes_on_stack + rsp;

    text_bytes_on_stack += write_to_stack(&instruction_stream, "99 Bottles");
    int offset_bottles = -text_bytes_on_stack + rsp;

    /*** parse file ***/
    while((instruction = fgetc(file)) != EOF)
    {
        switch (instruction)
        {
            case 'H':
                {
                    // access single chars of int
                    char *hw = (char*) &offset_hello_world;
                    char opcodes [] = {
                        // TODO call printf
                        0x8B, 0xBD, hw[0], hw[1], hw[2], hw[3], // mov -0x<offset>(%rbp), %edi
                        0x41, 0xFF, 0xD4 // callq *%r12
                    };
                    vector_push(&instruction_stream, opcodes, sizeof(opcodes));
                }
                break;

            case 'Q':
                {
                    // access single chars of int
                    char *s = (char*) &offset_source;
                    char opcodes [] = {
                        // TODO call printf
                        0x8B, 0xBD, s[0], s[1], s[2], s[3], // mov -0x<offset>(%rbp), %edi
                        0x41, 0xFF, 0xD4 // callq *%r12
                    };
                    vector_push(&instruction_stream, opcodes, sizeof(opcodes));
                }
                break;

            case '9':
                {
                    // access single chars of int
                    char *b = (char*) &offset_bottles;
                    char opcodes [] = {
                        // TODO call printf
                        0x8B, 0xBD, b[0], b[1], b[2], b[3], // mov -0x<offset>(%rbp), %edi
                        // 0xBF, 0x39, 0x00, 0x00, 0x00, // mov $0x39, %edi
                        0x41, 0xFF, 0xD4 // callq *%r12
                    };
                    vector_push(&instruction_stream, opcodes, sizeof(opcodes));
                }
                break;

            case '+':
                {
                    char opcodes [] = {
                        // increment the accumulator
                        0x48, 0xFF, 0x45, 0xF0, // incq -0x10(%rbp)

                        // call putchar(the_accumulator)
                        0x8B, 0x7D, 0xF0, //  mov -0x10(%rbp), %edi
                        0x41, 0xFF, 0xD4 // callq *%r12
                    };
                    vector_push(&instruction_stream, opcodes, sizeof(opcodes));
                }
                break;
        }
    }
    if (!feof(file)) {
        perror("Error reading file");
    }
    fclose(file);


    /*** epilogue ***/
    // access single chars of long
    char *t = (char*) &text_bytes_on_stack;
    char epilogue [] = {
        // free strings
        0x48, 0x81, 0xC4, t[0], t[1], t[2], t[3], // addq $<x>, %rsp
        // 0x48, 0x83, 0xC4, 0x08, // addq $8, %rsp

        // free accumulator
        0x48, 0x83, 0xC4, 0x08, // addq $8, %rsp
        // 0x48, 0x81, 0xC4, 0x08, 0x00, 0x00, 0x00, // addq $8, %rsp

        // restore callee saved registers
        0x41, 0x5C, // popq %r12

        0x5d, // pop rbp
        0xC3 // ret
    };
    vector_push(&instruction_stream, epilogue, sizeof(epilogue));


    /*** invoke generated code ***/
    /* allocate and copy instruction stream into executable memory */
    void* mem = mmap(NULL, instruction_stream.size, PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    memcpy(mem, instruction_stream.data, instruction_stream.size);

    /* typecast memory to a function pointer and call the dynamically created executable code */
    void (*hq9p_program) (fn_putchar) = mem;
    hq9p_program(putchar);

    /* clear up */
    munmap(mem, instruction_stream.size);
    vector_destroy(&instruction_stream);

    exit(EXIT_SUCCESS);
}

int write_to_stack(struct vector* const stream, char* text)
{
    int additional_bytes_on_stack = 0;

    // push on stack in reverse order
    int i;
    for (i = strlen(text)-1; i >= 0; --i) {
        // TODO 8 byte at once?
        char push_char [] = {
            0x6a, text[i]  // pushq $<char>
        };
        vector_push(stream, push_char, sizeof(push_char));
        additional_bytes_on_stack += (sizeof(push_char) - sizeof(char)) * sizeof(long);
    }

    return additional_bytes_on_stack;
}
