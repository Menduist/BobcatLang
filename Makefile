NAME=bob

SRCS=src/bob.c \
src/parser/parser.c \
src/tokenizer/tokenizer.c \
src/utils.c \
src/vector.c \
src/semantics/semantics.c \
src/semantics/sem_typefinder.c \
src/cgenerator/cgenerator.c \
src/llvm/llvm.c \

OBJS=$(addprefix objs/, $(SRCS:.c=.o))

CFLAGS= -rdynamic -Wall -Wextra -g

.PHONY: all fclean clean tests re

all: $(NAME)

$(NAME): $(OBJS)
	g++ $(OBJS) $(CFLAGS) -o $(NAME) `llvm-config --libs --ldflags --system-libs all` -lffi

-include objs/dep

objs/src/llvm/llvm.o: src/llvm/llvm.c
	@mkdir -p $(dir $@)
	gcc `llvm-config --cflags` $(CFLAGS) -c $< -o $@

objs/%.o: %.c
	@mkdir -p $(dir $@)
	@(printf $(dir $@) "/" && gcc -MM $<) >> objs/dep
	gcc $(CFLAGS) -c $< -o $@

clean:
	rm -fr $(OBJS)
	rm -fr objs/

fclean: clean
	rm -f $(NAME)

re: fclean all

tests: re
	cd tests; ./tests.sh
