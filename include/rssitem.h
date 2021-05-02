#ifndef NEWSBOAT_RSSITEM_H_
#define NEWSBOAT_RSSITEM_H_

#include <memory>
#include <mutex>
#include <string>

#include "matchable.h"
#include "matcher.h"
#include "utf8string.h"

namespace newsboat {

class Cache;
class RssFeed;

struct Description {
	std::string text;
	std::string mime;
};

struct InnerDescription {
	Utf8String text;
	Utf8String mime;
};

class RssItem : public Matchable {
public:
	explicit RssItem(Cache* c);
	~RssItem() override;

	std::string title() const
	{
		return title_.to_utf8();
	}
	void set_title(const std::string& t);

	/// \brief Feed's canonical URL. Empty if feed was never fetched.
	// FIXME(utf8): change this back to const std::string&
	std::string link() const
	{
		return link_.to_utf8();
	}
	void set_link(const std::string& l);

	std::string author() const
	{
		return author_.to_utf8();
	}
	void set_author(const std::string& a);

	Description description() const
	{
		std::lock_guard<std::mutex> guard(description_mutex);
		if (description_.has_value()) {
			Description result;
			result.text = description_.value().text.to_utf8();
			result.mime = description_.value().mime.to_utf8();
			return result;
		}
		return {"", ""};
	}
	void set_description(const std::string& content, const std::string& mime_type);

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

	// FIXME(utf8): change this back to const std::string&
	std::string guid() const
	{
		return guid_.to_utf8();
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
		feedurl_ = Utf8String::from_utf8(f);
	}

	// FIXME(utf8): change this back to const std::string&
	std::string feedurl() const
	{
		return feedurl_.to_utf8();
	}

	// FIXME(utf8): change this back to const std::string&
	std::string enclosure_url() const
	{
		return enclosure_url_.to_utf8();
	}
	// FIXME(utf8): change this back to const std::string&
	std::string enclosure_type() const
	{
		return enclosure_type_.to_utf8();
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

	nonstd::optional<std::string> attribute_value(const std::string& attr) const
	override;

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
		base = Utf8String::from_utf8(b);
	}
	// FIXME(utf8): change this back to const std::string&
	std::string get_base()
	{
		return base.to_utf8();
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
		std::lock_guard<std::mutex> guard(description_mutex);
		description_.reset();
	}

private:
	Utf8String title_;
	Utf8String link_;
	Utf8String author_;
	Utf8String guid_;
	Utf8String feedurl_;
	Cache* ch;
	Utf8String enclosure_url_;
	Utf8String enclosure_type_;
	std::string flags_;
	std::string oldflags_;
	std::weak_ptr<RssFeed> feedptr_;
	Utf8String base;
	unsigned int idx;
	unsigned int size_;
	time_t pubDate_;
	bool unread_;
	bool enqueued_;
	bool deleted_;
	bool override_unread_;

	mutable std::mutex description_mutex;
	nonstd::optional<InnerDescription> description_;
};

} // namespace newsboat

#endif /* NEWSBOAT_RSSITEM_H_ */
