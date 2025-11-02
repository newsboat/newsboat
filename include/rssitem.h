#ifndef NEWSBOAT_RSSITEM_H_
#define NEWSBOAT_RSSITEM_H_

#include <memory>
#include <mutex>
#include <string>

#include "matchable.h"
#include "matcher.h"

namespace newsboat {

class Cache;
class RssFeed;

struct Description {
	std::string text;
	std::string mime;
};

class RssItem : public Matchable {
public:
	explicit RssItem(Cache* c);
	~RssItem() override = default;

	std::string title() const
	{
		return "Item title";
	}
	void set_title(const std::string& /*t*/) {}

	std::string link() const
	{
		return "https://example.com/item";
	}
	void set_link(const std::string& /*l*/) {}

	std::string author() const
	{
		return "Charles Dickens";
	}
	void set_author(const std::string& /*a*/) {}

	Description description() const
	{
		return {"", ""};
	}
	void set_description(const std::string& /*content*/, const std::string& /*mime_type*/) {}

	unsigned int size() const
	{
		return 1024;
	}
	void set_size(unsigned int /*size*/) {}

	std::string length() const {return "1K"; }
	std::string pubDate() const {return "Mon, 01 Jan 2025 11:12:13 +03:00"; }

	time_t pubDate_timestamp() const
	{
		return 0;
	}
	void set_pubDate(time_t /*t*/) {}

	bool operator<(const RssItem& item) const
	{
		return this < &item;
	}

	const std::string& guid() const
	{
		return guid_;
	}
	void set_guid(const std::string& g);

	bool unread() const
	{
		return true;
	}
	void set_unread(bool u);
	void set_unread_nowrite(bool /*u*/) {}
	void set_unread_nowrite_notify(bool u, bool notify);

	void set_cache(Cache* c)
	{
		ch = c;
	}
	void set_feedurl(const std::string& f)
	{
		feedurl_ = f;
	}

	const std::string& feedurl() const;
	std::string enclosure_url() const {return "";}
	std::string enclosure_type() const {return "";}
	std::string enclosure_description() const {return "";}
	std::string enclosure_description_mime_type() const {return "";}

	void set_enclosure_url(const std::string& /*url*/) {}
	void set_enclosure_type(const std::string& /*type*/) {}
	void set_enclosure_description(const std::string& /*description*/) {}
	void set_enclosure_description_mime_type(const std::string& /*type*/) {}

	bool enqueued()
	{
		return false;
	}
	void set_enqueued(bool /*v*/)
	{
	}

	std::string flags() const
	{
		return "";
	}
	std::string oldflags() const
	{
		return "";
	}
	void set_flags(const std::string& /*ff*/) {}
	void update_flags() {}
	void sort_flags() {}

	std::optional<std::string> attribute_value(const std::string& attr) const
	override;

	void set_feedptr(std::shared_ptr<RssFeed> ptr);
	void set_feedptr(const std::weak_ptr<RssFeed>& ptr);
	std::shared_ptr<RssFeed> get_feedptr()
	{
		return feedptr_.lock();
	}

	bool deleted() const
	{
		return false;
	}
	void set_deleted(bool /*b*/)
	{
	}

	void set_index(unsigned int i)
	{
		idx = i;
	}

	void set_base(const std::string& /*b*/) {}
	std::string get_base() const
	{
		return "base";
	}

	void set_override_unread(bool /*b*/)
	{
	}
	bool override_unread()
	{
		return false;
	}

	void unload()
	{
	}

private:
	std::string guid_;
	std::string feedurl_;
	Cache* ch;
	std::weak_ptr<RssFeed> feedptr_;
	unsigned int idx;
};

} // namespace newsboat

#endif /* NEWSBOAT_RSSITEM_H_ */
