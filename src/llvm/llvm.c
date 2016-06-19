#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>

#include "llvm.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

struct llvm {

};

void llvm_interpret(struct ast_node *node) {
	struct llvm out;

	LLVMModuleRef mod = LLVMModuleCreateWithName("my_module");
}
