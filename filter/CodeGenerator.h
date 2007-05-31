#ifndef FILTER_PARSER__H
#define FILTER_PARSER__H

#include <string>

enum { LOGOP_AND = 1, LOGOP_OR, MATCHOP_EQ, MATCHOP_NE, MATCHOP_RXEQ, MATCHOP_RXNE };

struct expression {
	expression(const std::string& n, const std::string& lit, int o) : name(n), literal(lit), op(o), l(NULL), r(NULL), parent(NULL) { 
		/*
		if (literal.length() >= 2) { // remove quotes
			literal.erase(0);
			literal.erase(literal.length()-2);
		}
		*/
	}
	expression(int o) : op(o), l(NULL), r(NULL), parent(NULL) { }

	std::string name;
	std::string literal;
	int op;
	expression * l, * r;
	expression * parent;
};

class FilterParser {
	public:
		FilterParser();
		~FilterParser();
		void add_logop(int op);
		void add_matchexpr(char * name, int op, char * lit);
		void open_block();
		void close_block();

		bool parse_string(const std::string& str);
		void cleanup();

		void print_tree();

	private:
		void print_tree_r(expression * e, unsigned int depth);
		void cleanup_r(expression * e);

		expression * root;
		expression * curpos;
		bool next_must_descend_right;
};


#endif
