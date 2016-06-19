#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>

#include "llvm.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct llvm {
	LLVMModuleRef module;
	LLVMBuilderRef builder;
	LLVMValueRef main;
};

typedef LLVMValueRef (*node_llvm)(struct llvm *, struct ast_node *);

node_llvm gens[NODES_END];

static LLVMValueRef gen_node(struct llvm *llvm, struct ast_node *node) {
	LLVMValueRef ret;
	int i;

	if (gens[node->type] != NULL) {
		ret = gens[node->type](llvm, node);
		return ret;
	}
	if (node->type < TOKEN_LAST)
		goto end;

	for (i = 0; i < node->childcount; i++) {
		gen_node(llvm, node->childs[i]);
	}
end:
	return 0;
}

LLVMValueRef llvm_string_literal(struct llvm *llvm, struct ast_node *node) {
	char result[10000];
	char *base;
	int i, y;

	base = ((struct simple_token *)node)->value;
	for (i = 1, y = 0; base[i] != '"'; i++, y++) {
		if (base[i] == '\\') {
			i++;
			switch (base[i]) {
			case 'n':
				result[y] = '\n';
				break;
			case '\\':
				result[y] = '\\';
				break;
			}
		}
		else {
			result[y] = base[i];
		}
	}
	result[y] = 0;
	return LLVMBuildGlobalStringPtr(llvm->builder, result, "sl");
}

LLVMValueRef llvm_func_call(struct llvm *llvm, struct ast_node *node) {
	struct sem_function *sem = node->sem_val;
	LLVMValueRef args[sem->args.count];
	int i;

	for (i = 0; i < sem->args.count; i++) {
		args[i] = gen_node(llvm, node->childs[2]->childs[i]);
	}
	return LLVMBuildCall(llvm->builder, sem->gendata, args, sem->args.count,
			"");
}

LLVMValueRef llvm_func_def(struct llvm *llvm, struct ast_node *node) {
	struct sem_function *sem = node->sem_val;

	LLVMValueRef func = sem->gendata;

	LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, "entry");
	LLVMPositionBuilderAtEnd(llvm->builder, block);

	gen_node(llvm, node->childs[1]);

	LLVMBuildRetVoid(llvm->builder);
	return func;
}

void llvm_init_scope(struct llvm *llvm, struct sem_scope *scope) {
	int i;

	for (i = 0; i < scope->types.count; i++) {
		if (strcmp(scope->types.data[i]->name, "char") == 0) scope->types.data[i]->gendata = LLVMInt8Type();
		if (strcmp(scope->types.data[i]->name, "int") == 0) scope->types.data[i]->gendata = LLVMInt32Type();
		if (strcmp(scope->types.data[i]->name, "string") == 0) scope->types.data[i]->gendata = LLVMPointerType(LLVMInt8Type(), 0);
	}

	for (i = 0; i < scope->functions.count; i++) {
		struct sem_function *sem = scope->functions.data[i];
		LLVMTypeRef argsTypes[sem->args.count];
		int y;

		if (strcmp(sem->name, "prints") == 0) sem->name = "puts";

		for (y = 0; y < sem->args.count; y++) {
			argsTypes[y] = sem->args.data[y]->datatype->gendata;
		}

		LLVMTypeRef proto = LLVMFunctionType(sem->result_type ? sem->result_type->gendata : LLVMVoidType(),
				argsTypes,
				sem->args.count,
				0);
		LLVMValueRef val = LLVMAddFunction(llvm->module, sem->name, proto);
		sem->gendata = val;
		if (strcmp(sem->name, "main") == 0) llvm->main = val;
	}
}

void llvm_interpret(struct ast_node *node) {
	struct llvm out;

	out.module = LLVMModuleCreateWithName("m");
	out.builder = LLVMCreateBuilder();

	llvm_init_scope(&out, node->sem_val);
	gen_node(&out, node);

	char *error = NULL;
	LLVMVerifyModule(out.module, LLVMAbortProcessAction, &error);
	LLVMDisposeMessage(error);

	if (LLVMWriteBitcodeToFile(out.module, "sum.bc") != 0) {
		fprintf(stderr, "error writing bitcode to file, skipping\n");
	}

	LLVMExecutionEngineRef engine;
	error = NULL;
	LLVMLinkInInterpreter();
	if (LLVMCreateExecutionEngineForModule(&engine, out.module, &error) != 0) {
		fprintf(stderr, "failed to create execution engine\n");
	}
	if (error) {
		fprintf(stderr, "error: %s\n", error);
		LLVMDisposeMessage(error);
		exit(EXIT_FAILURE);
	}
	LLVMRunFunction(engine, out.main, 0, 0);

	LLVMDisposeBuilder(out.builder);
}

void init_llvm(void) {
	memset(gens, 0, sizeof(gens));

	gens[FUNCTION_DEFINITION] = llvm_func_def;
	gens[FUNCTION_CALL] = llvm_func_call;

	gens[TOKEN_STRING_LITERAL] = llvm_string_literal;
}
