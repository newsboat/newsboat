#include "CodeGenerator.h"
#include "Parser.h"
#include <sstream>

FilterParser::FilterParser() : root(0) { }

FilterParser::~FilterParser() { }

void FilterParser::add_logop(int op) { 
	expression * expr = new expression(op);
	if (!root) {
			// ERROR: there can't be a logical expression w/o temporary (left) expression
	} else {
		expr->l = root;
		root = expr;
	}
	// printf("logop: %d\n", op); 
}

void FilterParser::add_matchexpr(char * name, int op, char * lit) { 
	expression * expr = new expression(name, lit, op);
	if (!root) {
		root = expr;
	} else {
		root->r = expr;
	}
	coco_string_delete(name);
	coco_string_delete(lit);
	// printf("matchexpr: %ls lit = %ls op = %d\n", name, lit, op); 
}

void FilterParser::open_block() { 
	//printf("openblock\n"); 
	// TODO
}

void FilterParser::close_block() { 
	//printf("closeblock\n"); 
	// TODO
}

void FilterParser::parse_string(const std::string& str) {
	std::istringstream is(str);
	Scanner s(is);
	Parser p(&s);
	p.gen = this;
	p.Parse();
}

void FilterParser::print_tree() {
	print_tree_r(root,0);
}

void FilterParser::print_tree_r(expression * e, unsigned int depth) {
	if (e) {
		print_tree_r(e->l, depth + 1);
		for (unsigned int i=0;i<depth;++i) {
			printf("\t");
		}
		printf("op = %u name = %s literal = %s\n", e->op, e->name.c_str(), e->literal.c_str());
		print_tree_r(e->r, depth + 1);
	}
}
