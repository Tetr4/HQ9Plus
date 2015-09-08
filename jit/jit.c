#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include "vector.h"

/*
!!! Work in Progress !!!
TODO: no errors/warnings on 'gcc -ansi -pedantic -Wall jit.c vector.c -o jit'
TODO: functionality for H,Q,9 and + instructions

JIT-Compiler for HQ9+ files (http://esolangs.org/wiki/HQ9)

H: Print "hello, world"
Q: Print the program's source code
9: Print the lyrics to "99 Bottles of Beer"
+: Increment the accumulator

Compile the jit compiler:
    gcc jit.c vector.c -o jit

Jit a program:
    ./jit ../main.hq9+
*/

/* libc function as argument, for easy access */
typedef int fn_putchar (int);

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

    vector_create(&instruction_stream, 100);

    /* prologue */
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

    /* parse file */
    while((instruction = fgetc(file)) != EOF)
    {
        switch (instruction)
        {
            case 'H':
                {
                    char opcodes [] = {
                        // TODO call printf with address of "hello world" as param
                        // call putchar('H')
                        0xBF, 0x48, 0x00, 0x00, 0x00, // mov $0x48, %edi
                        0x41, 0xFF, 0xD4 // callq *%r12
                    };
                    vector_push(&instruction_stream, opcodes, sizeof(opcodes));
                }
                break;

            case 'Q':
                {
                    char opcodes [] = {
                        // TODO call printf with address of source code as param
                        // call putchar('Q')
                        0xBF, 0x51, 0x00, 0x00, 0x00, // mov $0x51, %edi
                        0x41, 0xFF, 0xD4 // callq *%r12
                    };
                    vector_push(&instruction_stream, opcodes, sizeof(opcodes));
                }
                break;

            case '9':
                {
                    char opcodes [] = {
                        // TODO call printf with address of lyrics as param
                        // call putchar('9')
                        0xBF, 0x39, 0x00, 0x00, 0x00, // mov $0x39, %edi
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

    /* epilogue */
    char epilogue [] = {
        // free accumulator
        0x48, 0x83, 0xC4, 0x08, // addq $8, %rsp
        // 0x48, 0x81, 0xC4, 0x08, 0x00, 0x00, 0x00, // addq $8, %rsp

        // restore callee saved registers
        0x41, 0x5C, // popq %r12

        0x5d, // pop rbp
        0xC3 // ret
    };
    vector_push(&instruction_stream, epilogue, sizeof(epilogue));

    /* allocate and copy instruction stream into executable memory */
    void* mem = mmap(NULL, instruction_stream.size, PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    memcpy(mem, instruction_stream.data, instruction_stream.size);

    /* typecast memory to a function pointer and invoke the dynamically created executable code */
    void (*hq9p_program) (fn_putchar) = mem;
    hq9p_program(putchar);

    /* clear up */
    munmap(mem, instruction_stream.size);
    vector_destroy(&instruction_stream);

    exit(EXIT_SUCCESS);
}
