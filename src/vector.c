#include "vector.h"
#include <stdlib.h>
#include <string.h>

void _vector_init(struct internal_vector *vec, int startsize, int datasize)
{
    vec->data = malloc(startsize * datasize);
    vec->count = 0;
    vec->limit = startsize;
    vec->data_size = datasize;
}

void *_vector_append(struct internal_vector *vector, void *data)
{
    int i;

    i = vector->count++;
    if (vector->count == vector->limit) {
        vector->limit *= 3;
        vector->data = realloc(vector->data, vector->limit * vector->data_size);
    }
    if (data != NULL) {
        memcpy(vector->data + i * vector->data_size, data, vector->data_size);
    }
    return vector->data + i * vector->data_size;
}

void vector_delete(struct internal_vector *vector)
{
    free(vector->data);
    memset(vector, 0, sizeof(struct internal_vector));
}

void vector_delete_values(struct internal_vector *vector)
{
	int i;

	for (i = 0; i < vector->count; i++)
		free(((void **)vector->data)[i]);
	vector_delete(vector);
}
