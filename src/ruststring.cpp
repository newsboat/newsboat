#include "ruststring.h"

namespace Newsboat {

extern "C" void rs_cstring_free(char* str);

RustString::RustString(char* ptr)
	: str(ptr)
{
}

RustString::operator std::string()
{
	if (str != nullptr) {
		return std::string(str);
	}
	return std::string();
}

RustString::~RustString()
{
	// This pointer is checked for nullptr on the rust side.
	rs_cstring_free(str);
}

} /* Newsboat */
