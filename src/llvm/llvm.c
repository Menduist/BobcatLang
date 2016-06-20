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
#include <ctype.h>

enum wanted_value {
	DEFAULT,
	LVALUE,
	RVALUE
};

struct llvm {
	LLVMModuleRef module;
	LLVMBuilderRef builder;
	LLVMValueRef main;
	enum wanted_value wanted_value;
};

typedef LLVMValueRef (*node_llvm)(struct llvm *, struct ast_node *);

node_llvm gens[NODES_END];

static LLVMValueRef gen_node(struct llvm *llvm, struct ast_node *node) {
	LLVMValueRef ret = 0;
	int i;

	if (gens[node->type] != NULL) {
		ret = gens[node->type](llvm, node);
		return ret;
	}
	if (node->type < TOKEN_LAST)
		goto end;

	for (i = 0; i < node->childcount; i++) {
		ret = gen_node(llvm, node->childs[i]);
	}
end:
	return ret;
}

static LLVMValueRef get_lvalue(struct llvm *llvm, struct ast_node *node) {
	enum wanted_value backup = llvm->wanted_value;
	LLVMValueRef result;

	llvm->wanted_value = LVALUE;
	result = gen_node(llvm, node);
	llvm->wanted_value = backup;
	return result;
}

static LLVMValueRef get_rvalue(struct llvm *llvm, struct ast_node *node) {
	enum wanted_value backup = llvm->wanted_value;
	LLVMValueRef result;

	llvm->wanted_value = RVALUE;
	result = gen_node(llvm, node);
	llvm->wanted_value = backup;
	return result;
}

LLVMValueRef llvm_identifier(struct llvm *llvm, struct ast_node *node) {
	if (node->sem_val) {
		if (llvm->wanted_value == LVALUE)
			return ((struct sem_variable *) node->sem_val)->gendata;
		return LLVMBuildLoad(llvm->builder, ((struct sem_variable *) node->sem_val)->gendata, "loadres");
	}
	if (isdigit(((struct simple_token *)node)->value[0])) {
		int val = atoi(((struct simple_token *)node)->value);
		return LLVMConstInt(LLVMInt32Type(), val, 1);
	}
	printf("wtf %s\n", ((struct simple_token *)node)->value);
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

LLVMValueRef llvm_operator(struct llvm *llvm, struct ast_node *node) {
	char *op = ((struct simple_token *)node->childs[0])->value;

	if (strcmp(op, "=") == 0) {
		return LLVMBuildStore(llvm->builder, gen_node(llvm, node->childs[2]),
				get_lvalue(llvm, node->childs[1]));
	}
	else if (strcmp(op, "*") == 0) {
		return LLVMBuildMul(llvm->builder, gen_node(llvm, node->childs[1]),
				gen_node(llvm, node->childs[2]), "mulres");
	}
	else if (strcmp(op, "/") == 0) {
		return LLVMBuildSDiv(llvm->builder, gen_node(llvm, node->childs[1]),
				gen_node(llvm, node->childs[2]), "mulres");
	}
	else if (strcmp(op, "==") == 0) {
		return LLVMBuildICmp(llvm->builder, LLVMIntEQ,
				gen_node(llvm, node->childs[1]),
				gen_node(llvm, node->childs[2]), "cmpres");
	}
	printf("unhandled operator %s\n", op);
	return 0;
}

LLVMValueRef llvm_prefix_operator(struct llvm *llvm, struct ast_node *node) {
	char *op = ((struct simple_token *)node->childs[0])->value;

	if (strcmp(op, "++") == 0) {
		LLVMValueRef var = get_lvalue(llvm, node->childs[1]);
		LLVMValueRef result = LLVMBuildLoad(llvm->builder, var, "inc");
		LLVMBuildStore(llvm->builder,
				LLVMBuildAdd(llvm->builder,
					result,
					LLVMConstInt(LLVMInt32Type(), 1, 1), "inced"),
				var);

		return result;
	}
	printf("unhandled prefix operator %s\n", op);
	return 0;
}

LLVMValueRef llvm_if(struct llvm *llvm, struct ast_node *node) {
	LLVMValueRef cond = gen_node(llvm, node->childs[1]);

	LLVMValueRef currentBlock = LLVMGetBasicBlockParent(LLVMGetInsertBlock(llvm->builder));

	LLVMBasicBlockRef thenBlock = LLVMAppendBasicBlock(currentBlock, "then");
	LLVMBasicBlockRef elseBlock = LLVMAppendBasicBlock(currentBlock, "else");
	LLVMBasicBlockRef mergeBlock = LLVMAppendBasicBlock(currentBlock, "ifend");

	LLVMBuildCondBr(llvm->builder, cond, thenBlock, elseBlock);

	/*
	** Then
	*/
	LLVMPositionBuilderAtEnd(llvm->builder, thenBlock);
	gen_node(llvm, node->childs[2]);

	LLVMBuildBr(llvm->builder, mergeBlock);

	/*
	** Else
	*/
	LLVMPositionBuilderAtEnd(llvm->builder, elseBlock);
	if (node->childcount > 3)
		gen_node(llvm, node->childs[3]);
	LLVMBuildBr(llvm->builder, mergeBlock);

	LLVMPositionBuilderAtEnd(llvm->builder, mergeBlock);
	return 0;
}

LLVMValueRef getprintf(struct llvm *llvm) {
	static LLVMValueRef res = 0;

	if (res)
		return res;

	LLVMTypeRef args[] = { LLVMPointerType(LLVMInt8Type(), 0) };
	LLVMTypeRef proto = LLVMFunctionType(LLVMInt32Type(),
			args,
			1,
			1);
	res = LLVMAddFunction(llvm->module, "printf", proto);
	return res;
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

		LLVMValueRef prinf = getprintf(llvm);
		if (strcmp(sem->name, "prints") == 0) {
			LLVMBasicBlockRef block = LLVMAppendBasicBlock(val, "entry");
			LLVMPositionBuilderAtEnd(llvm->builder, block);

			LLVMValueRef params[] = { LLVMBuildGlobalStringPtr(llvm->builder, "%s", "printfcall"),
				LLVMGetParam(val, 0) };
			LLVMBuildCall(llvm->builder, prinf, params, 2, "printf");
			LLVMBuildRetVoid(llvm->builder);
		}
		if (strcmp(sem->name, "printi") == 0) {
			LLVMBasicBlockRef block = LLVMAppendBasicBlock(val, "entry");
			LLVMPositionBuilderAtEnd(llvm->builder, block);

			LLVMValueRef params[] = { LLVMBuildGlobalStringPtr(llvm->builder, "%d\n", "printfcall"),
				LLVMGetParam(val, 0) };
			LLVMBuildCall(llvm->builder, prinf, params, 2, "printf");
			LLVMBuildRetVoid(llvm->builder);
		}

	}

	for (i = scope->implicit_var_count; i < scope->variables.count; i++) {
		struct sem_variable *var = scope->variables.data[i];

		var->gendata = LLVMBuildAlloca(llvm->builder, var->datatype->gendata, var->name);
		LLVMBuildStore(llvm->builder, LLVMConstInt(var->datatype->gendata, 0, 1), var->gendata);
	}
}

LLVMValueRef llvm_coumpound_statement(struct llvm *llvm, struct ast_node *node) {
	int i;

	llvm_init_scope(llvm, node->sem_val);
	for (i = 0; i < node->childcount; i++)
		gen_node(llvm, node->childs[i]);
	return 0;
}

LLVMValueRef llvm_variable_declaration(struct llvm *llvm, struct ast_node *node) {
	int i;

	for (i = 0; i < node->childcount; i++)
		if (node->childs[i]->type != TOKEN_IDENTIFIER)
			gen_node(llvm, node->childs[i]);
	return 0;
}

void llvm_interpret(struct ast_node *node) {
	struct llvm out;

	memset(&out, 0, sizeof(struct llvm));
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
	gens[COMPOUND_STATEMENT] = llvm_coumpound_statement;
	gens[OPERATOR] = llvm_operator;
	gens[PREFIX_OPERATOR] = llvm_prefix_operator;
	gens[VARIABLE_DECLARATION] = llvm_variable_declaration;
	gens[IF_STATEMENT] = llvm_if;

	gens[TOKEN_STRING_LITERAL] = llvm_string_literal;
	gens[TOKEN_IDENTIFIER] = llvm_identifier;
}
