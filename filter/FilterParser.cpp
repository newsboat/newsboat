#include "FilterParser.h"

#include "logger.h"
#include "Parser.h"
#include "utils.h"

using namespace Newsboat;

FilterParser::FilterParser() : root(0), curpos(0), next_must_descend_right(false) { }

FilterParser::~FilterParser() {
	cleanup();
}

FilterParser::FilterParser(const FilterParser& p) {
	LOG(Level::DEBUG,"FilterParser: copy constructor called!");
	parse_string(p.strexpr);
}

FilterParser& FilterParser::operator=(FilterParser& p) {
	LOG(Level::DEBUG,"FilterParser: operator= called!");
	if (this != &p) {
		cleanup();
		parse_string(p.strexpr);
	}
	return *this;
}

void FilterParser::add_logop(int op) {
	expression * expr = new expression(op);
	if (!root) {
		printf("error: there can't be a logical expression w/o a prior expression!");
		delete expr;
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
}

void FilterParser::add_matchexpr(char * name, int op, char * lit) {
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
}

void FilterParser::open_block() {
	next_must_descend_right = true;
}

void FilterParser::close_block() {
	if (curpos != root) {
		curpos = curpos->parent;
	}
}

bool FilterParser::parse_string(const std::string& str) {
	cleanup();
	strexpr = str;

	Scanner s((const unsigned char *)str.c_str(), str.length());
	Parser p(&s);
	p.gen = this;
	p.Parse();
	if (0 == p.errors->count) {
		return true;
	}
	errmsg = p.errors->errors[0];
	cleanup();
	return false;
}

void FilterParser::cleanup() {
	cleanup_r(root);
	root = curpos = NULL;
}

void FilterParser::cleanup_r(expression * e) {
	if (e) {
		LOG(Level::DEBUG,"cleanup_r: e = %p", e);
		cleanup_r(e->l);
		cleanup_r(e->r);
		delete e;
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
