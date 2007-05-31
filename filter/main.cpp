#include <iostream>
#include "CodeGenerator.h"

int main(void) {
	FilterParser codegen;

	codegen.parse_string("foo == \"bar\" and ( x == \"y\" or y != \"x\" )");
	codegen.print_tree();
}
