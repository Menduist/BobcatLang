#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>

void print_backtrace(void);
void *allocfailed(size_t nmemb, size_t size);

static inline void *memalloc_(size_t nmemb, size_t size) {
	void *result = calloc(nmemb, size);
	if (!result) {
		(void) allocfailed(nmemb, size);
	}
	return result;
}

#define memalloc(type, count) (type *)memalloc_(sizeof(type), count)
#endif
