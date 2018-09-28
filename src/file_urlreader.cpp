#include "file_urlreader.h"

#include <fstream>

#include "utils.h"

namespace newsboat {

file_urlreader::file_urlreader(const std::string& file)
	: filename(file)
{
}

std::string file_urlreader::get_source()
{
	return filename;
}

void file_urlreader::reload()
{
	urls.clear();
	tags.clear();
	alltags.clear();

	std::fstream f;
	f.open(filename.c_str(), std::fstream::in);
	if (f.is_open()) {
		std::string line;
		while (!f.eof()) {
			std::getline(f, line);
			if (line.length() > 0 && line[0] != '#') {
				std::vector<std::string> tokens =
					utils::tokenize_quoted(line);
				if (!tokens.empty()) {
					std::string url = tokens[0];
					urls.push_back(url);
					tokens.erase(tokens.begin());
					if (!tokens.empty()) {
						tags[url] = tokens;
						for (const auto& token :
							tokens) {
							alltags.insert(token);
						}
					}
				}
			}
		};
	}
}

void file_urlreader::load_config(const std::string& file)
{
	filename = file;
	reload();
}

void file_urlreader::write_config()
{
	std::fstream f;
	f.open(filename.c_str(), std::fstream::out);
	if (f.is_open()) {
		for (const auto& url : urls) {
			f << url;
			if (tags[url].size() > 0) {
				for (const auto& tag : tags[url]) {
					f << " \"" << tag << "\"";
				}
			}
			f << std::endl;
		}
	}
}

}
