#include "fileurlreader.h"

#include <fstream>

#include "utils.h"

namespace newsboat {

FileUrlReader::FileUrlReader(const std::string& file)
	: filename(file)
{
}

std::string FileUrlReader::get_source()
{
	return filename;
}

void FileUrlReader::reload()
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
					Utils::tokenize_quoted(line);
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

void FileUrlReader::load_config(const std::string& file)
{
	filename = file;
	reload();
}

void FileUrlReader::write_config()
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
