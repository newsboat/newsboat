#ifndef UTIL_H_
#define UTIL_H_

#include <vector>
#include <string>

namespace noos {

class utils {
	public:
		static std::vector<std::string> tokenize(const std::string& str, std::string delimiters = " \r\n\t");
};

}

#endif /*UTIL_H_*/
