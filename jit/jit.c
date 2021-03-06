#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include "vector.h"

/*
TODO: no errors/warnings on 'gcc -ansi -pedantic -Wall jit.c vector.c -o jit'

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
    gcc -nostartfiles -o assembly_test assembly_code.s && objdump -s assembly_test
*/

void write_to_stack(struct vector* const vec, char* text, int* stack_offset);
char* get_source_code(char* filename);
char* get_lyrics(int initial_bottle_count);

/* libc function as assembly argument, for easy access */
typedef int fn_printf (const char *, ...);

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

    int stack_offset = -0x10; // offset from %rbp
    int offset_accumulator = stack_offset; // accumulator address: -0x10(%rbp)

    // hello world
    write_to_stack(&instruction_stream, "Hello World\n", &stack_offset);
    int offset_hello_world = stack_offset;

    // source code
    char* source_code = get_source_code(filename);
    write_to_stack(&instruction_stream, source_code, &stack_offset);
    free(source_code);
    int offset_source = stack_offset;

    // lyrics
    char* lyrics = get_lyrics(99);
    write_to_stack(&instruction_stream, lyrics, &stack_offset);
    free(lyrics);
    int offset_bottles = stack_offset;

    // everything after accumulator is text bytes
    int text_bytes_on_stack = -(stack_offset - offset_accumulator);


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

void write_to_stack(struct vector* const stream, char* text, int* stack_offset)
{
    /*
        Calculate and allocate required stack space (8 aligned, with \0 terminator),
        then push quads (8 byte) on stack in reverse order. Finally fill incomplete
        quad with zeros (which act as \0 terminator) or add quad with \0 terminator.

        string_length; required_space_aligned; example
         0    8   "00000000"    <- \0 terminator + 7 zeros
         1    8   "H0000000"
         2    8   "He000000"
         3    8   "Hel00000"
         4    8   "Hell0000"
         5    8   "Hello000"
         6    8   "Hello 00"
         7    8   "Hello W0"
         8   16   "00000000" "Hello Wo"    <- 1 additional quad for \0 terminator
         9   16   "r0000000" "Hello Wo"
        10   16   "rl000000" "Hello Wo"
        11   16   "rld00000" "Hello Wo"
        12   16   ...
        13   16   ...
        14   16   ...
        15   16   ...
        16   24   ...    <- 1 additional quad for \0 terminator
        17   24   ...
        18   24   ...
        19   24   ...
    */

    // calculate stack space
    int length = strlen(text);
    int required_space_aligned = length + 8 - (length) % 8;

    // allocate stack space
    char *s = (char*) &required_space_aligned;
    char allocate [] = {
        0x48, 0x81, 0xEC, s[0], s[1], s[2], s[3] // subq $0x<required_space_aligned>, %rsp
    };
    vector_push(stream, allocate, sizeof(allocate));
    *stack_offset -= required_space_aligned;

    /*
        push chars in quads on the stack (in reverse order, e.g. ... "rld00000" "hello wo")

        pushing immediate with pushq is not supported or leaves zeros:
        https://stackoverflow.com/questions/13351363/push-on-64bit-intel-osx

        thus move into register first, then move register to stack:
        movq $0x6f57206f6c6c6548, %rax
        movq %rax,-0x20(%rbp)
    */
    int write_pos = *stack_offset; // where to write on stack
    int chars_written = 0;
    char* char_ptr;
    for (char_ptr = text; *char_ptr != '\0'; char_ptr++){
        if(chars_written % 8 == 0) {
            vector_push_byte(stream, 0x48); // movq $<char1, char2, ..., char8>, %rax
            vector_push_byte(stream, 0xB8);
        }

        vector_push_byte(stream, *char_ptr); // $<char1, char2, ..., char8>
        chars_written++;

        if(chars_written % 8 == 0) {
            char *pos = (char*) &write_pos;
            char mov_to_stack [] = {
                0x48, 0x89, 0x85, pos[0], pos[1], pos[2], pos[3] // movq %rax,<write_pos>(%rbp)
            };
            vector_push(stream, mov_to_stack, sizeof(mov_to_stack));
            write_pos += 8; // move stack up by 1 quad
        }
    }

    // if just completed pushing a quad, need to add '\0' terminator
    if(chars_written % 8 == 0) {
        char terminator [] = {
            0x48, 0xB8, '\0' // movq $0x00, %rax
        };
        vector_push(stream, terminator, sizeof(terminator));
        chars_written++;
    }

    // if quad not completely filled, fill remaining bytes with 0. will also act as '\0' terminator
    if(chars_written %8 != 0) {
        while(chars_written % 8 != 0) {
            vector_push_byte(stream, '\0');
            chars_written++;
        }
        char *pos = (char*) &write_pos;
        char mov_to_stack [] = {
            0x48, 0x89, 0x85, pos[0], pos[1], pos[2], pos[3] // mov %rax,<write_pos>(%rbp)
        };
        vector_push(stream, mov_to_stack, sizeof(mov_to_stack));
    }

}

char* get_source_code(char* filename)
{
    FILE *file;
    long length;
    char *buffer;

    /* Open file again for second independent seek point indicator.
       Read only -> No problem to have same file open multiple times. */
    file = fopen(filename, "r");
    if (file == NULL)
    {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    /* get file length */
    fseek(file , 0L , SEEK_END);
    length = ftell(file);
    rewind(file);

    /* allocate memory for file content */
    buffer = malloc(length+1);
    if(buffer == NULL) {
         fclose(file);
         perror("Error allocating memory for source code");
         exit(EXIT_FAILURE);
    }

    /* copy the file into the buffer */
    if(fread(buffer, 1, length, file) != length) {
        fclose(file);
        free(buffer);
        perror("Could not read file into buffer");
        exit(EXIT_FAILURE);
    }

    /* Close second file descriptor */
    fclose(file);

    return buffer;
}

char* get_lyrics(int initial_bottle_count)
{
    int bottle_count;
    char* pluralized_bottle;

    const int MAX_LINE_LENGTH = 256; // just a guess
    char line_buffer[MAX_LINE_LENGTH];
    int chars_to_write;
    struct vector lyrics_stream;
    char* buffer;

    vector_create(&lyrics_stream, 100);

    // create formated strings and put them into a vector.
    for (bottle_count = initial_bottle_count; bottle_count >= 0; bottle_count--)
    {
        if (bottle_count > 0)
        {
            pluralized_bottle = bottle_count == 1 ? "bottle" : "bottles";
            chars_to_write = snprintf(line_buffer, MAX_LINE_LENGTH, "%d %s of beer on the wall, %d %s of beer.\n" \
                     "Take one down and pass it around, ", bottle_count, pluralized_bottle, bottle_count, pluralized_bottle);
            vector_push(&lyrics_stream, line_buffer, chars_to_write);
            if (bottle_count == 1)
            {
                chars_to_write = snprintf(line_buffer, MAX_LINE_LENGTH, "no more bottles of beer on the wall.\n");
                vector_push(&lyrics_stream, line_buffer, chars_to_write);
            }
            else
            {
                pluralized_bottle = bottle_count-1 == 1 ? "bottle" : "bottles";
                chars_to_write = snprintf(line_buffer, MAX_LINE_LENGTH, "%d %s of beer on the wall.\n", bottle_count-1, pluralized_bottle);
                vector_push(&lyrics_stream, line_buffer, chars_to_write);
            }
        }
        else
        {
            pluralized_bottle = initial_bottle_count == 1 ? "bottle" : "bottles";
            chars_to_write = snprintf(line_buffer, MAX_LINE_LENGTH, "No more bottles of beer on the wall, no more bottles of beer.\n" \
            "Go to the store and buy some more, " \
            "%d %s of beer on the wall.\n", initial_bottle_count, pluralized_bottle);
            vector_push(&lyrics_stream, line_buffer, chars_to_write);
        }
    }

    // copy data to buffer so the vector can be freed.
    buffer = malloc(lyrics_stream.size);
    memcpy(buffer, lyrics_stream.data, lyrics_stream.size);

    vector_destroy(&lyrics_stream);

    return buffer;
}
