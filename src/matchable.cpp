#include "matchable.h"

#include <stdio.h>
#include <string.h>

using namespace newsboat;

bool matchable_has_attribute(const void *ptr, const char *attr) {
	Matchable *matchable = (Matchable*) ptr;
	return matchable->has_attribute(attr);
}

char* matchable_get_attribute(const void *ptr, const char *attr) {
	Matchable *matchable = (Matchable*) ptr;
	std::string stdstr = matchable->get_attribute(attr);
	const char *str = stdstr.c_str();
	char *mem = (char*) malloc(stdstr.size() + 1);
	strncpy(mem, str, stdstr.size() + 1);
	return mem;
}
