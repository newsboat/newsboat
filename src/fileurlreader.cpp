#include "fileurlreader.h"

#include <cstring>
#include <fstream>
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
	feed_urls.emplace_back(FeedUrl{url, FeedOrigin{}, url_tags});
}

std::optional<utils::ReadTextFileError> FileUrlReader::reload()
{
	feed_urls.clear();

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

		const std::string url = tokens[0];
		tokens.erase(tokens.begin());

		auto it = std::find_if(feed_urls.begin(), feed_urls.end(),
		[&url](const FeedUrl& u) {
			return u.url == url;
		});
		if (it == feed_urls.end()) {
			feed_urls.emplace_back(FeedUrl{url, FeedOrigin{FileOrigin{line_number}}, tokens});
		} else {
			std::string warn_msg = strprintf::fmt(
					_("Warning: Duplicate URL found: %s. Merging tags."),
					url);

			LOG(Level::USERERROR, warn_msg.c_str());
			std::cerr << warn_msg << std::endl;

			for (const std::string& tag : tokens) {
				if (std::find(it->tags.begin(), it->tags.end(), tag) == it->tags.end()) {
					it->tags.push_back(tag);
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
	for (auto& feed_url : feed_urls) {
		line_number++;
		f << utils::quote_if_necessary(feed_url.url);
		for (const auto& tag : feed_url.tags) {
			f << " \"" << tag << "\"";
		}
		f << std::endl;

		// Update origin as writing to urls file might remove comments and empty lines,
		// resulting in URLs ending up at different line numbers
		feed_url.origin = FeedOrigin{FileOrigin{line_number}};
	}

	return {};
}

}
