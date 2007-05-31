#include <iostream>
#include "CodeGenerator.h"

int main(void) {
	CodeGenerator codegen;

	codegen.parse_string("foo == \"bar\" and ( x == \"y\" or y != \"x\" )");
}
