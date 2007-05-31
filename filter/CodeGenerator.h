#ifndef FILTER_CODEGEN__H
#define FILTER_CODEGEN__H

#include "Parser.h"
#include <cstdio>

class CodeGenerator {
	public:
		CodeGenerator() { }
		~CodeGenerator() { }
		void add_logop(int op) { printf("logop: %d\n", op); }
		void add_matchexpr(wchar_t * name, int op, wchar_t * lit) { printf("matchexpr: %ls lit = %ls op = %d\n", name, lit, op); }
		void open_block() { printf("openblock\n"); }
		void close_block() { printf("closeblock\n"); }
};


#endif
