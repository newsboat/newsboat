#ifndef NEWSBOAT_RSSITEM_H_
#define NEWSBOAT_RSSITEM_H_

#include <memory>
#include <string>

#include "matchable.h"
#include "matcher.h"

namespace newsboat {

class Cache;
class RssFeed;

class RssItem : public Matchable {
public:
	explicit RssItem(Cache* c);
	~RssItem() override;

	std::string title() const
	{
		return title_;
	}
	void set_title(const std::string& t);

	/// \brief Feed's canonical URL. Empty if feed was never fetched.
	const std::string& link() const
	{
		return link_;
	}
	void set_link(const std::string& l);

	std::string author() const
	{
		return author_;
	}
	void set_author(const std::string& a);

	std::string description() const
	{
		return description_.value_or("");
	}
	void set_description(const std::string& d);

	unsigned int size() const
	{
		return size_;
	}
	void set_size(unsigned int size);

	std::string length() const;
	std::string pubDate() const;

	time_t pubDate_timestamp() const
	{
		return pubDate_;
	}
	void set_pubDate(time_t t);

	bool operator<(const RssItem& item) const
	{
		return item.pubDate_ < this->pubDate_; // new items come first
	}

	const std::string& guid() const
	{
		return guid_;
	}
	void set_guid(const std::string& g);

	bool unread() const
	{
		return unread_;
	}
	void set_unread(bool u);
	void set_unread_nowrite(bool u);
	void set_unread_nowrite_notify(bool u, bool notify);

	void set_cache(Cache* c)
	{
		ch = c;
	}
	void set_feedurl(const std::string& f)
	{
		feedurl_ = f;
	}

	const std::string& feedurl() const
	{
		return feedurl_;
	}

	const std::string& enclosure_url() const
	{
		return enclosure_url_;
	}
	const std::string& enclosure_type() const
	{
		return enclosure_type_;
	}

	void set_enclosure_url(const std::string& url);
	void set_enclosure_type(const std::string& type);

	bool enqueued()
	{
		return enqueued_;
	}
	void set_enqueued(bool v)
	{
		enqueued_ = v;
	}

	const std::string& flags() const
	{
		return flags_;
	}
	const std::string& oldflags() const
	{
		return oldflags_;
	}
	void set_flags(const std::string& ff);
	void update_flags();
	void sort_flags();

	nonstd::optional<std::string> attribute_value(const std::string& attr) override;

	void set_feedptr(std::shared_ptr<RssFeed> ptr);
	void set_feedptr(const std::weak_ptr<RssFeed>& ptr);
	std::shared_ptr<RssFeed> get_feedptr()
	{
		return feedptr_.lock();
	}

	bool deleted() const
	{
		return deleted_;
	}
	void set_deleted(bool b)
	{
		deleted_ = b;
	}

	void set_index(unsigned int i)
	{
		idx = i;
	}
	unsigned int get_index()
	{
		return idx;
	}

	void set_base(const std::string& b)
	{
		base = b;
	}
	const std::string& get_base()
	{
		return base;
	}

	void set_override_unread(bool b)
	{
		override_unread_ = b;
	}
	bool override_unread()
	{
		return override_unread_;
	}

	void unload()
	{
		description_.reset();
	}

private:
	std::string title_;
	std::string link_;
	std::string author_;
	nonstd::optional<std::string> description_;
	std::string guid_;
	std::string feedurl_;
	Cache* ch;
	std::string enclosure_url_;
	std::string enclosure_type_;
	std::string flags_;
	std::string oldflags_;
	std::weak_ptr<RssFeed> feedptr_;
	std::string base;
	unsigned int idx;
	unsigned int size_;
	time_t pubDate_;
	bool unread_;
	bool enqueued_;
	bool deleted_;
	bool override_unread_;
};

} // namespace newsboat

#endif /* NEWSBOAT_RSSITEM_H_ */
