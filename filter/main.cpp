#include <iostream>
#include "CodeGenerator.h"

int main(void) {
	FilterParser codegen;

//	codegen.parse_string("foo == \"bar\" and ( x == \"y\" or y != \"x\" )");
//	codegen.parse_string("( foo == \"bar\" )");
	codegen.parse_string("( a == \"b\" and c == \"d\" ) or ( e != \"f\" and g =~ \"h\" ) and i !~ \"j\"");
	codegen.print_tree();
}
