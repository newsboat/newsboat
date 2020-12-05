#include "fileurlreader.h"

#include <cstring>
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

nonstd::optional<std::string> FileUrlReader::reload()
{
	urls.clear();
	tags.clear();
	alltags.clear();

	std::vector<std::string> lines;
	std::string error_message;
	if (!utils::read_text_file(filename, lines, error_message)) {
		return strprintf::fmt(_("Error: Failed to read URLs from file \"%s\" (%s)"),
				filename,
				error_message);
	}

	for (const std::string& line : lines) {
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

	return {};
}

nonstd::optional<std::string> FileUrlReader::write_config()
{
	std::fstream f;
	f.open(filename, std::fstream::out);
	if (!f.is_open()) {
		const auto error_message = strerror(errno);
		return strprintf::fmt(_("Error: Failed to open file \"%s\" (%s)"),
				filename,
				error_message);
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

	return {};
}

}
