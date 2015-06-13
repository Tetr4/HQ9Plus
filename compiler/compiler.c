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
void print_escaped_source_code(char* filename);
void print_escaped_bottles_of_beer(int initial_bottle_count);

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
    fputs(
      ".data\n"
      
      "hello:\n"
      "  .asciz \"hello world\\n\"\n"
      
      "source:\n"
      "  .asciz \""
    , stdout);
    print_escaped_source_code(filename);
    puts("\"");
    
    fputs(
      "bottles:\n"
      "  .asciz \""
    , stdout);
    print_escaped_bottles_of_beer(99);
    puts("\"");
    
    puts(
      "accumulator:\n"
      "  .long 0\n"
    );

    /* text segment */
    puts(
      ".text\n"
      ".globl _start\n"
      
      "H:\n"
      /* call printf("hello world\n") */
      /* don't use printf varargs -> store 0 as vararg length */
      "  movb $0, %al\n"
      /* load effective address of string relativ to
         instruction pointer (%rip) as first argument (%rdi).
         Argument sequence for function calls:
         %rdi, %rsi, %rdx, %rcx, %r8, %r9 then on the stack in reverse order */
      "  leaq hello(%rip), %rdi\n"
      "  call printf\n"
      "  ret\n"
      
      "Q:\n"
      "  movb $0, %al\n"
      "  leaq source(%rip), %rdi\n"
      "  call printf\n"
      "  ret\n"
      
      "Nine:\n"
      /* call printf("%d bottles of beer\n", accumulator) */
      /* TODO assembly loop for whole song */
      "  movb $1, %al\n"
      "  leaq bottles(%rip), %rdi\n"
      /* accumulator as second argument for now*/
      "  movq accumulator(%rip), %rsi\n"
      "  call printf\n"
      "  ret\n"
      
      "Plus:\n"
      "  incq accumulator(%rip)\n"
      "  ret\n"
      
      "_start:\n"
      /* align stack pointer (%rsp) with multiple of 16 Byte. 
         Stack starts at high address and grows down -> subtract
         suffix: b - Byte / 8  bit
                 w - Word / 16 bit
                 l - Long / 32 bit
                 q - Quad / 64 bit <- x86_64 */
      "  subq $8,  %rsp"
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
                puts("  call H");
                break;
                
            case 'Q':
                puts("  call Q");
                break;
                
            case '9':
                puts("  call Nine");
                break;
                
            case '+':
                puts("  call Plus");
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


void print_escaped_source_code(char* filename)
{
    int c;
    FILE *file;
    
    /* Open file again for second independent seek point indicator.
       Read only -> No problem to have same file open multiple times. */
    file = fopen(filename, "r");
    if (file == NULL)
    {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }
    
    /* Print source code */
    while ((c = getc(file)) != EOF)
    {
        /* escape special characters */
        switch (c)
        {
            case '\a':  fputs("\\a", stdout); break;
            case '\b':  fputs("\\b", stdout); break;
            case '\f':  fputs("\\f", stdout); break;
            case '\n':  fputs("\\n", stdout); break;
            case '\r':  fputs("\\r", stdout); break;
            case '\t':  fputs("\\t", stdout); break;
            case '\v':  fputs("\\v", stdout); break;
            case '\\':  fputs("\\\\", stdout); break;
            case '\'':  fputs("\\'", stdout); break;
            case '\"':  fputs("\\\"", stdout); break;
            case '\?':  fputs("\\\?", stdout); break;
            default:
                putchar(c);
        }
    }
    
    /* Close second file descriptor */
    fclose(file);
}

void print_escaped_bottles_of_beer(int initial_bottle_count)
{
    /* TODO only text required for assembly loop */
    int bottle_count;
    char* pluralized_bottle;

    for (bottle_count = initial_bottle_count; bottle_count >= 0; bottle_count--)
    {
        if (bottle_count > 0)
        {
            pluralized_bottle = bottle_count == 1 ? "bottle" : "bottles";
            printf("%d %s of beer on the wall, %d %s of beer.\\n" \
                     "Take one down and pass it around, ", bottle_count, pluralized_bottle, bottle_count, pluralized_bottle);
            if (bottle_count == 1)
            {
                printf("no more bottles of beer on the wall.\\n");
            }
            else
            {
                pluralized_bottle = bottle_count-1 == 1 ? "bottle" : "bottles";
                printf("%d %s of beer on the wall.\\n", bottle_count-1, pluralized_bottle);
            }
        }
        else
        {
            pluralized_bottle = initial_bottle_count == 1 ? "bottle" : "bottles";
            printf("No more bottles of beer on the wall, no more bottles of beer.\\n" \
            "Go to the store and buy some more, " \
            "%d %s of beer on the wall.\\n", initial_bottle_count, pluralized_bottle);
        }
    }
}
