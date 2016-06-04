#include "utils.h"
#include <execinfo.h>
#include <stdio.h>

void print_backtrace(void) {
	int c, i;
	void *addresses[30];
	char **strings; 

	c = backtrace(addresses, 30);
	strings = backtrace_symbols(addresses,c);
	printf("backtrace returned: %dn", c);
	for(i = 0; i < c; i++) {
		printf("%d: %p ", i, addresses[i]);
		printf("%sn\n", strings[i]);
	}
}

void *allocfailed(size_t nmemb, size_t size) {
	fprintf(stderr, "memalloc failed with %lu,%lu\n", nmemb, size);
	print_backtrace();
	exit(-1);
}

extern inline void *memalloc_(size_t nmemb, size_t size);
