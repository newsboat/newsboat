#ifndef NEWSBOAT_LINKS_H_
#define NEWSBOAT_LINKS_H_
#include <vector>
#include <utility>
#include <string>
namespace newsboat {
// This enum has to be kept in sync with enum LinkType in rust/libnewsboat/src/links.rs
enum class LinkType { HREF, IMG, EMBED, IFRAME, VIDEO, AUDIO };

typedef std::pair<std::string, LinkType> LinkPair;

class Links {
public:
	using iterator = std::vector<LinkPair>::iterator;
	using const_iterator = std::vector<LinkPair>::const_iterator;

	unsigned int add_link(const std::string& url, LinkType type);

	LinkPair& operator[] (size_t idx)
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

private:
	std::vector<LinkPair> links;
};
}
#endif
