#include <utils.h>

namespace newsbeuter {

std::vector<std::string> utils::tokenize_config(const std::string& str, std::string delimiters) {
	std::vector<std::string> tokens = tokenize(str,delimiters);
	for (std::vector<std::string>::iterator it=tokens.begin();it!=tokens.end();++it) {
		if ((*it)[0] == '#') {
			tokens.erase(it,tokens.end());
			break;
		}
	}
	return tokens;
}
	
std::vector<std::string> utils::tokenize(const std::string& str, std::string delimiters) {
    std::vector<std::string> tokens;
    std::string::size_type last_pos = str.find_first_not_of(delimiters, 0);
    std::string::size_type pos = str.find_first_of(delimiters, last_pos);

    while (std::string::npos != pos || std::string::npos != last_pos) {
            tokens.push_back(str.substr(last_pos, pos - last_pos));
            last_pos = str.find_first_not_of(delimiters, pos);
            pos = str.find_first_of(delimiters, last_pos);
    }
    return tokens;
}


}
