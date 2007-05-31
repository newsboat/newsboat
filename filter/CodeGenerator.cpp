#include "CodeGenerator.h"
#include "Parser.h"
#include <sstream>

FilterParser::FilterParser() : root(0), curpos(0), next_must_descend_right(false) { }

FilterParser::~FilterParser() { }

void FilterParser::add_logop(int op) { 
	//fprintf(stderr,"add_logop: op = %d\n", op);
	expression * expr = new expression(op);
	if (!root) {
		printf("error: there can't be a logical expression w/o a prior expression!");
		// TODO: add proper error handling
	} else {
		if (curpos != root) {
			expr->l = curpos;
			curpos->parent->r = expr;
			expr->parent = curpos->parent;
			curpos = expr;
		} else {
			expr->l = root;
			curpos = root = expr;
		}
	}
	// printf("logop: %d\n", op); 
}

void FilterParser::add_matchexpr(char * name, int op, char * lit) { 
	//fprintf(stderr,"add_matchexpr: name = %s op = %d lit = %s\n", name, op, lit);
	expression * expr = new expression(name, lit, op);
	if (next_must_descend_right) {
		next_must_descend_right = false;
		if (!curpos) {
			curpos = root = expr;
		} else {
			expr->parent = curpos;
			curpos->r = expr;
			curpos = expr;
		}
	} else {
		if (!curpos) {
			curpos = root = expr;
		} else {
			expr->parent = curpos;
			curpos->r = expr;
		}
	}
	coco_string_delete(name);
	coco_string_delete(lit);
	// printf("matchexpr: %ls lit = %ls op = %d\n", name, lit, op); 
}

void FilterParser::open_block() { 
	//fprintf(stderr,"open_block\n");
	next_must_descend_right = true;
}

void FilterParser::close_block() { 
	//fprintf(stderr,"close_block\n");
	if (curpos != root) {
		curpos = curpos->parent;
	}
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
