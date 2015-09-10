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
typedef int fn_printf (const char *, ...);
void write_to_stack(struct vector* const vec, char* text, int* stack_pointer);

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
        0x6a, 0x00, // pushq $0
    };
    vector_push(&instruction_stream, prologue, sizeof(prologue));

    int stack_pointer = -0x10; // (%rsp)
    int offset_accumulator = stack_pointer; // accumulator address: -0x10(%rbp)

    write_to_stack(&instruction_stream, "Hello World", &stack_pointer);
    int offset_hello_world = stack_pointer;

    write_to_stack(&instruction_stream, "Q", &stack_pointer);
    int offset_source = stack_pointer;

    write_to_stack(&instruction_stream, "99 Bottles", &stack_pointer);
    int offset_bottles = stack_pointer;


    // everything after accumulator is text bytes
    int text_bytes_on_stack = -(stack_pointer - offset_accumulator);


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
                        0xB0, 00, // movb $0, %al
                        0x48, 0x8D, 0xBD, hw[0], hw[1], hw[2], hw[3], // leaq -0x<offset>(%rbp),%rdi
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
                        0xB0, 00, // movb $0, %al
                        0x48, 0x8D, 0xBD, s[0], s[1], s[2], s[3], // leaq -0x<offset>(%rbp),%rdi
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
                        0xB0, 00, // movb $0, %al
                        0x48, 0x8D, 0xBD, b[0], b[1], b[2], b[3], // leaq -0x<offset>(%rbp),%rdi
                        // 0xBF, 0x39, 0x00, 0x00, 0x00, // mov $0x39, %edi
                        0x41, 0xFF, 0xD4 // callq *%r12
                    };
                    vector_push(&instruction_stream, opcodes, sizeof(opcodes));
                }
                break;

            case '+':
                {
                    char *acc = (char*) &offset_accumulator;
                    char opcodes [] = {
                        // increment the accumulator
                        // TODO from variable instead of constant offset
                        0x48, 0xFF, 0x45, 0xF0, // incq -0x10(%rbp)
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

        // free accumulator
        0x48, 0x83, 0xC4, 0x08, // addq $8, %rsp

        // restore callee saved register
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
    void (*hq9p_program) (fn_printf) = mem;
    hq9p_program(printf);

    /* clear up */
    munmap(mem, instruction_stream.size);
    vector_destroy(&instruction_stream);

    exit(EXIT_SUCCESS);
}

void write_to_stack(struct vector* const stream, char* text, int* stack_pointer)
{

    /*
        push chars in quads (8 byte) on the stack

        pushing immediate with pushq is not supported or leaves zeros:
        https://stackoverflow.com/questions/13351363/push-on-64bit-intel-osx

        thus move into register first:
        movq $0x6f57206f6c6c6548, %rax
        pushq %rax
    */
    char* char_ptr;
    for (char_ptr = text; *char_ptr != '\0'; char_ptr++){
        if(*stack_pointer % 8 == 0) {
            vector_push_byte(stream, 0x48); // movq $<char1, char2, ..., char8>, %rax
            vector_push_byte(stream, 0xB8);
        }

        vector_push_byte(stream, *char_ptr); // $<char1, char2, ..., char8>
        *stack_pointer -= sizeof(char);

        if(*stack_pointer % 8 == 0) {
                vector_push_byte(stream, 0x50); // pushq %rax
        }
    }

    // TODO how to deal with terminator '\0'

    // FIXME zeros act as \0
    // check if quad completely filled
    if(*stack_pointer %8 != 0) {
        // fill remaining bytes with 0
        while(*stack_pointer % 8 != 0) {
            vector_push_byte(stream, 0x00);
            *stack_pointer -= sizeof(char);
        }
        vector_push_byte(stream, 0x50); // pushq %rax
    }

}
