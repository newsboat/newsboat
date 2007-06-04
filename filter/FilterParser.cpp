#include "logger.h"
#include "FilterParser.h"
#include "Parser.h"
#include <sstream>

using namespace newsbeuter;

FilterParser::FilterParser() : root(0), curpos(0), next_must_descend_right(false) { }

FilterParser::~FilterParser() { 
	cleanup();
}

FilterParser::FilterParser(const FilterParser& p) {
	GetLogger().log(LOG_DEBUG,"FilterParser: copy constructor called!");
	parse_string(p.strexpr);
}

FilterParser& FilterParser::operator=(FilterParser& p) {
	GetLogger().log(LOG_DEBUG,"FilterParser: operator= called!");
	if (this != &p) {
		cleanup();
		parse_string(p.strexpr);
	}
	return *this;
}

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

bool FilterParser::parse_string(const std::string& str) {
	cleanup();
	strexpr = str;

	std::istringstream is(str);
	Scanner s(is);
	Parser p(&s);
	p.gen = this;
	p.Parse();
	if (0 == p.errors->count) {
		return true;
	}
	cleanup();
	return false;
}

void FilterParser::cleanup() {
	cleanup_r(root);
	root = curpos = NULL;
}

void FilterParser::cleanup_r(expression * e) {
	if (e) {
		GetLogger().log(LOG_DEBUG,"cleanup_r: e = %p", e);
		cleanup_r(e->l);
		cleanup_r(e->r);
		delete e;
	}
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

expression::expression(const std::string& n, const std::string& lit, int o) : name(n), literal(lit), op(o), l(NULL), r(NULL), parent(NULL), regex(NULL) {
	if (literal[0] == '"' && literal[literal.length()-1] == '"') {
		literal = literal.substr(1,literal.length()-2);
	}
}

expression::expression(int o) : op(o), l(NULL), r(NULL), parent(NULL), regex(NULL) { 
}

expression::~expression() {
	if (regex)
		regfree(regex);
	delete regex;
}
