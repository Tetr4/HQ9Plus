#include <stdlib.h>
#include <string.h>
#include "vector.h"

void vector_create (struct vector* const vec, size_t capacity) {
    vec->size = 0;
    vec->capacity = capacity;
    vec->data = malloc(capacity * sizeof(char));
}

void vector_destroy (struct vector* vec) {
    if (vec->data != NULL) {
        free(vec->data);
    }
}

void vector_push (struct vector* const vec, char* bytes, size_t len) {
    if (vec->size + len > vec->capacity) {
        vec->capacity *= 2;
        vec->data = realloc(vec->data, vec->capacity * sizeof(char));
    }
    memcpy(vec->data + vec->size, bytes, len);
    vec->size += len;
}

void vector_push_byte (struct vector* const vec, char byte) {
    vector_push(vec, &byte, 1);
}
