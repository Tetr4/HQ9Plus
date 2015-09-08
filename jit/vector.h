struct vector {
  size_t size;
  size_t capacity;
  char* data;
};

void vector_create (struct vector* const vec, size_t capacity);
void vector_destroy (struct vector* vec);
void vector_push (struct vector* const vec, char* bytes, size_t len);
