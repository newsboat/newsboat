#include "fileurlwriter.h"

#include <cstring>
#include <fstream>

#include "config.h"
#include "strprintf.h"

namespace newsboat {

std::optional<std::string> FileUrlWriter::write_urls(const std::vector<UrlReader::FeedUrl>&
	feed_urls, const Filepath& filename)
{
	std::fstream f;
	f.open(filename.to_locale_string(), std::fstream::out);
	if (!f.is_open()) {
		const auto error_message = strerror(errno);
		return strprintf::fmt(_("Error: failed to open file \"%s\": %s"),
				filename,
				error_message);
	}

	for (auto& feed_url : feed_urls) {
		f << utils::quote_if_necessary(feed_url.url);
		for (const auto& tag : feed_url.tags) {
			f << " \"" << tag << "\"";
		}
		f << std::endl;
	}

	return {};
}

}
