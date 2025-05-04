#include "urlreader.h"

#include <set>

namespace newsboat {

std::vector<std::string>& UrlReader::get_urls()
{
	return urls;
}

std::vector<std::string>& UrlReader::get_tags(const std::string& url)
{
	return tags[url];
}

std::vector<std::string> UrlReader::get_alltags()
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

} // namespace newsboat
