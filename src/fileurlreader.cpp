#include "fileurlreader.h"

#include <cstring>
#include <fstream>
#include <unordered_map>
#include <set>
#include <vector>
#include <iostream>

#include "config.h"
#include "feedorigin.h"
#include "strprintf.h"
#include "utils.h"
#include "logger.h"

namespace newsboat {

FileUrlReader::FileUrlReader(const Filepath& file)
	: filename(file)
{
}

std::string FileUrlReader::get_source() const
{
	return filename.display();
}

void FileUrlReader::add_url(const std::string& url,
	const std::vector<std::string>& url_tags)
{
	urls.push_back({url, FeedOrigin{}});
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

	std::size_t line_number = 0;
	for (const std::string& line : lines) {
		line_number++;

		// skip empty lines and comments
		if (line.empty() || line[0] == '#') {
			continue;
		}

		std::vector<std::string> tokens = utils::tokenize_quoted(line);
		if (tokens.empty()) {
			continue;
		}

		std::string url = tokens[0];
		auto it = std::find_if(urls.begin(), urls.end(),
		[&url](const std::pair<std::string, FeedOrigin>& u) {
			return u.first == url;
		});
		if (it == urls.end()) {
			urls.push_back({url, FeedOrigin{FileOrigin{line_number}}});
			tokens.erase(tokens.begin());
			if (!tokens.empty()) {
				tags[url] = tokens;
			}
		} else {
			std::string warn_msg = strprintf::fmt(
					_("Warning: Duplicate URL found: %s. Merging tags."),
					url);

			LOG(Level::USERERROR, warn_msg.c_str());
			std::cerr << warn_msg << std::endl;

			tokens.erase(tokens.begin());
			for (const std::string& tag : tokens) {
				if (std::find(tags[url].begin(),
						tags[url].end(),
						tag) == tags[url].end()) {
					tags[url].push_back(tag);
				}
			}
		}
	}

	return {};
}

std::optional<std::string> FileUrlReader::write_config()
{
	std::fstream f;
	f.open(filename.to_locale_string(), std::fstream::out);
	if (!f.is_open()) {
		const auto error_message = strerror(errno);
		return strprintf::fmt(_("Error: failed to open file \"%s\": %s"),
				filename,
				error_message);
	}

	std::size_t line_number = 0;
	for (auto& [url, origin] : urls) {
		line_number++;
		f << utils::quote_if_necessary(url);
		if (tags[url].size() > 0) {
			for (const auto& tag : tags[url]) {
				f << " \"" << tag << "\"";
			}
		}
		f << std::endl;

		// Update origin as writing to urls file might remove comments and empty lines,
		// resulting in URLs ending up at different line numbers
		origin = FeedOrigin{FileOrigin{line_number}};
	}

	return {};
}

}
