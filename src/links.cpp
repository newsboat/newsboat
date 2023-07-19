#include "links.h"
#include "utils.h"
namespace newsboat {
unsigned int Links::add_link(const std::string& url, LinkType type)
{
	bool found = false;
	unsigned int i = 1;
	for (const auto& l : links) {
		if (l.url == newsboat::utils::censor_url(url)) {
			found = true;
			break;
		}
		i++;
	}

	if (!found) {
		links.push_back({newsboat::utils::censor_url(url), type});
	} else if (links[i - 1].type == LinkType::HREF) {
		links[i - 1].type = type;
	}

	return i;
}
}
