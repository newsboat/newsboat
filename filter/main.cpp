#include <iostream>
#include <cassert>
#include "FilterParser.h"

int main(void) {
	FilterParser codegen;

	codegen.parse_string("foo == \"bar\" and ( x == \"y\" or y != \"x\" )");
	codegen.print_tree();

	printf("\n=========================\n\n");

	codegen.parse_string("( foo == \"bar\" )");
	codegen.print_tree();

	printf("\n=========================\n\n");

	codegen.parse_string("( a == \"b\" and c == \"d\" ) or ( e != \"f\" and g =~ \"h\" ) and i !~ \"j\"");
	codegen.print_tree();

	assert(codegen.parse_string(" ( a ")==false);
	assert(codegen.parse_string("a == \"a\"")==true);
	assert(codegen.parse_string("a == \"a")==false);
	assert(codegen.parse_string("a==\"a\"")==true);
	assert(codegen.parse_string("(a=\"a\")and(b!=\"b\")or(c=~\"x\")")==true);
	assert(codegen.parse_string("(a=\"a\"and(b!=\"b\")or(c=~\"x\")")==false);
	assert(codegen.parse_string("(a=\"a\")and(b!=\"b\")or(c=~\"x\"))")==false);
	assert(codegen.parse_string("()()()()a=b()c=d()()()")==false);

	assert(codegen.parse_string("unread_count != \"0\"")==true);

	printf("\n\ntests finished.\n");
}
