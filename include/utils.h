#ifndef UTIL_H_
#define UTIL_H_

#include <vector>
#include <string>

namespace newsbeuter {

class utils {
	public:
		static std::vector<std::string> tokenize(const std::string& str, std::string delimiters = " \r\n\t");
		static std::vector<std::string> tokenize_spaced(const std::string& str, std::string delimiters = " \r\n\t");
		static std::vector<std::string> tokenize_config(const std::string& str, std::string delimiters = " \r\n\t");
};

}

#endif /*UTIL_H_*/
