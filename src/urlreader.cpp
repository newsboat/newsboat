#include "urlreader.h"

#include <set>

#include "fileurlreader.h"
#include "logger.h"

namespace newsboat {

const std::vector<std::string>& UrlReader::get_urls() const
{
	return urls;
}

std::vector<std::string>& UrlReader::get_tags(const std::string& url)
{
	return tags[url];
}

std::vector<std::string> UrlReader::get_alltags() const
{
	std::set<std::string> tmptags;
	for (const auto& url_tags : tags) {
		for (const auto& tag : url_tags.second) {
			if (tag.substr(0, 1) != "~") {
				// std::copy_if would make this code less readable IMHO
				// cppcheck-suppress useStlAlgorithm
				tmptags.insert(tag);
			}
		}
	}
	return std::vector<std::string>(tmptags.begin(), tmptags.end());
}

void UrlReader::load_query_urls_from_file(Filepath file)
{
	FileUrlReader file_url_reader(file);
	const auto error_message = file_url_reader.reload();
	if (error_message.has_value()) {
		LOG(Level::DEBUG, "Reloading failed: %s", error_message.value().message);
		// Ignore errors for now: https://github.com/newsboat/newsboat/issues/1273
	}

	const auto& other_urls = file_url_reader.get_urls();
	for (const auto& url : other_urls) {
		if (utils::is_query_url(url)) {
			urls.push_back(url);
			tags[url] = file_url_reader.get_tags(url);
		}
	}
}

} // namespace newsboat
