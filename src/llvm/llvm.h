#ifndef LLVM_H
#define LLVM_H

#include "../semantics/semantics.h"

//void llvm_compile(struct ast_node *node);
void llvm_interpret(struct ast_node *node);

#endif
