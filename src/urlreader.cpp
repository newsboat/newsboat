#include "urlreader.h"

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
	std::vector<std::string> tmptags;
	for (const auto& t : alltags) {
		if (t.substr(0, 1) != "~") {
			// std::copy_if would make this code less readable IMHO
			// cppcheck-suppress useStlAlgorithm
			tmptags.push_back(t);
		}
	}
	return tmptags;
}

} // namespace newsboat
