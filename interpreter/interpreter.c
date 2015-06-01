#include <stdio.h>
#include <stdlib.h>

/*
Interpreter for HQ9+ files (http://esolangs.org/wiki/HQ9)

Compiling the interpreter:
    gcc -ansi -pedantic -Wall interpreter.c -o HQ9+

Using the interpreter:
    ./HQ9+ ../main.hq9+

H: Print "hello, world"
Q: Print the program's source code
9: Print the lyrics to "99 Bottles of Beer"
+: Increment the accumulator
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
    int the_accumulator = 0;
    
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
    
    /* parse file */
    while((instruction = fgetc(file)) != EOF)
    {
        switch (instruction)
        {
            case 'H':
                print_hello_world();
                break;
            case 'Q':
                print_source_code(filename);
                break;
            case '9':
                print_bottles_of_beer(99);
                break;
            case '+':
                increment_the_accumulator(&the_accumulator);
                break;
        }
    }
    if (!feof(file)) {
        perror("Error reading file");
    }
    
    /* close file */
    fclose(file);
    
    exit(EXIT_SUCCESS);
}



void print_hello_world()
{
    printf("hello, world\n");
}


void print_source_code(char* filename)
{
    /* TODO Buffer Data */
    int c;
    FILE *file;
    file = fopen(filename, "r");
    if (file) {
        while ((c = getc(file)) != EOF)
        {
            putchar(c);
        }
    fclose(file);
    }
}

void print_bottles_of_beer(int bottle_count)
{
    int original_bottle_count = bottle_count;
    char plural_s;
    for(; bottle_count >= 0; bottle_count--)
    {
        if(bottle_count > 0)
        {
            plural_s = bottle_count == 1 ? '\0' : 's';
            printf("%d bottle%c of beer on the wall, %d bottle%c of beer.\n" \
                     "Take one down and pass it around, ", bottle_count, plural_s, bottle_count, plural_s);
            if (bottle_count == 1)
            {
                printf("no more bottles of beer on the wall.\n");
            }
            else
            {
                plural_s = bottle_count-1 == 1 ? '\0' : 's';
                printf("%d bottle%c of beer on the wall.\n", bottle_count-1, plural_s);
            }
        }
        else
        {
            plural_s = original_bottle_count == 1 ? '\0' : 's';
            printf("No more bottles of beer on the wall, no more bottles of beer.\n" \
            "Go to the store and buy some more, " \
            "%d bottle%c of beer on the wall.\n", original_bottle_count, plural_s);
        }
    }
}


void increment_the_accumulator(int* accumulator)
{
    (*accumulator)++;
}

