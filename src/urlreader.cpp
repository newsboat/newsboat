#include "urlreader.h"

namespace newsboat {

std::vector<std::string>& urlreader::get_urls()
{
	return urls;
}

std::vector<std::string>& urlreader::get_tags(const std::string& url)
{
	return tags[url];
}

std::vector<std::string> urlreader::get_alltags()
{
	std::vector<std::string> tmptags;
	for (const auto& t : alltags) {
		if (t.substr(0, 1) != "~")
			tmptags.push_back(t);
	}
	return tmptags;
}

} // namespace newsboat
