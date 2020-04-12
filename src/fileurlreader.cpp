#include "fileurlreader.h"

#include <cerrno>
#include <fstream>
#include <system_error>

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
	f.open(filename, std::fstream::in);
	if (!f.is_open()) {
		throw std::system_error(errno, std::system_category(),
			strprintf::fmt(_("failed to open urls file (%s)"), filename));
	}

	for (std::string line; std::getline(f, line); /* nothing */) {
		// skip empty lines and comments
		if (line.empty() || line[0] == '#') {
			continue;
		}

		std::vector<std::string> tokens = utils::tokenize_quoted(line);
		if (tokens.empty()) {
			continue;
		}

		std::string url = tokens[0];
		urls.push_back(url);

		tokens.erase(tokens.begin());
		if (!tokens.empty()) {
			tags[url] = tokens;
			for (const auto& token : tokens) {
				alltags.insert(token);
			}
		}
	};
}

void FileUrlReader::load_config(const std::string& file)
{
	filename = file;
	reload();
}

void FileUrlReader::write_config()
{
	std::fstream f;
	f.open(filename, std::fstream::out);
	if (!f.is_open()) {
		throw std::system_error(errno, std::system_category(),
			strprintf::fmt(_("failed to open urls file (%s)"), filename));
	}

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
