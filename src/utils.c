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

char *readfile(char *path) {
	char *target;
	FILE *source;
	int size;
	
	source = fopen(path, "r");
	fseek(source, 0, SEEK_END);
	size = ftell(source);
	target = malloc(sizeof(char) * (size + 1));
	fseek(source, 0, SEEK_SET);
	fread(target, 1, size, source);
	target[size] = '\0';
	fclose(source);
	return target;
}

extern inline void *memalloc_(size_t nmemb, size_t size);
