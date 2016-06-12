#ifndef VECTOR_H
#define VECTOR_

struct internal_vector {
    void *data;
    int count;
    int limit;
    int data_size;
};

#define DECLAREVECTOR(name, type) struct name ## _vector { type *data; int count; int limit; int data_size; }

void _vector_init(struct internal_vector *vec, int startsize, int data_size);
#define vector_init(vec, startsize) _vector_init((struct internal_vector*)(vec), startsize, sizeof(*((vec)->data)))

void *_vector_append(struct internal_vector *vec, void *data);
#define vector_append(vec, datat) ((__typeof__(((vec)->data)))_vector_append((struct internal_vector*)(vec), datat))
#define vector_append_value(vec, datat) (*vector_append(vec, 0) = (datat))

//Implicit args: void vector_delete(struct internal_vector *vector);
void vector_delete();

DECLAREVECTOR(string, char *);

#endif
