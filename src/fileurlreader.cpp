#include "fileurlreader.h"

#include <cstring>
#include <fstream>

#include "config.h"
#include "strprintf.h"
#include "utils.h"

namespace newsboat {

FileUrlReader::FileUrlReader(const Filepath& file)
	: filename(file)
{
}

std::string FileUrlReader::get_source() const
{
	return filename;
}

void FileUrlReader::add_url(const std::string& url,
	const std::vector<std::string>& url_tags)
{
	urls.push_back(url);
	tags[url] = url_tags;
}

std::optional<utils::ReadTextFileError> FileUrlReader::reload()
{
	urls.clear();
	tags.clear();

	auto result = utils::read_text_file(filename);
	if (!result) {
		return result.error();
	}
	std::vector<std::string> lines = result.value();

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
		}
	};

	return {};
}

std::optional<std::string> FileUrlReader::write_config()
{
	std::fstream f;
	f.open(filename, std::fstream::out);
	if (!f.is_open()) {
		const auto error_message = strerror(errno);
		return strprintf::fmt(_("Error: failed to open file \"%s\": %s"),
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
