#include "CodeGenerator.h"
#include "Parser.h"
#include <sstream>

CodeGenerator::CodeGenerator() { }

CodeGenerator::~CodeGenerator() { }

void CodeGenerator::add_logop(int op) { 
	printf("logop: %d\n", op); 
}

void CodeGenerator::add_matchexpr(wchar_t * name, int op, wchar_t * lit) { 
	printf("matchexpr: %ls lit = %ls op = %d\n", name, lit, op); 
}

void CodeGenerator::open_block() { 
	printf("openblock\n"); 
}

void CodeGenerator::close_block() { 
	printf("closeblock\n"); 
}

void CodeGenerator::parse_string(const std::string& str) {
	std::istringstream is(str);
	Scanner s(is);
	Parser p(&s);
	p.gen = this;
	p.Parse();
}
