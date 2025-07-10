#ifndef NEWSBOAT_LINKS_H_
#define NEWSBOAT_LINKS_H_

#include <string>
#include <vector>

#include "config.h"

namespace Newsboat {

// This enum has to be kept in sync with enum LinkType in rust/libNewsboat/src/links.rs
enum class LinkType { HREF, IMG, EMBED, IFRAME, VIDEO, AUDIO };

struct LinkPair {
	std::string url;
	LinkType type;
};

class Links {
public:
	using iterator = std::vector<LinkPair>::iterator;
	using const_iterator = std::vector<LinkPair>::const_iterator;

	unsigned int add_link(const std::string& url, LinkType type);

	const LinkPair& operator[] (size_t idx) const
	{
		return links[idx];
	};

	iterator begin()
	{
		return links.begin();
	};
	iterator end()
	{
		return links.end();
	}

	const_iterator cbegin() const
	{
		return links.cbegin();
	}
	const_iterator cend() const
	{
		return links.cend();
	}

	size_t size() const
	{
		return links.size();
	}

	void clear()
	{
		links.clear();
	}

	bool empty() const
	{
		return links.empty();
	}

	static std::string type2str(LinkType type)
	{
		switch (type) {
		case LinkType::HREF:
			return _("link");
		case LinkType::IMG:
			return _("image");
		case LinkType::EMBED:
			return _("embedded flash");
		case LinkType::IFRAME:
			return _("iframe");
		case LinkType::VIDEO:
			return _("video");
		case LinkType::AUDIO:
			return _("audio");
		default:
			return _("unknown (bug)");
		}
	}

private:
	std::vector<LinkPair> links;
};

}
#endif
