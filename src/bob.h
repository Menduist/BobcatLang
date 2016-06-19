#ifndef BOB_H
#define BOB_H

enum bob_compile_mode {
	MODE_NONE,
	MODE_TOKENIZE,
	MODE_PARSE,
	MODE_SEM,
	MODE_INTER,
	MODE_COMPILE,
	MODE_EXECUTE,
	MODE_END
};

char *bob_compile_mode_names[] = {
	"none",
	"tokenize",
	"parse",
	"semantical",
	"interpret",
	"compile",
	"execute"
};

struct bob {
	enum bob_compile_mode mode;
	char *file;
	char *source;
};

#endif
