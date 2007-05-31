#ifndef FILTER_CODEGEN__H
#define FILTER_CODEGEN__H

#include <string>

class CodeGenerator {
	public:
		CodeGenerator();
		~CodeGenerator();
		void add_logop(int op);
		void add_matchexpr(wchar_t * name, int op, wchar_t * lit);
		void open_block();
		void close_block();

		void parse_string(const std::string& str);
};


#endif
