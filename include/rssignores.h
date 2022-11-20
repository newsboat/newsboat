#ifndef NEWSBOAT_RSSIGNORES_H_
#define NEWSBOAT_RSSIGNORES_H_

#include <vector>

#include "configactionhandler.h"
#include "matcher.h"
#include "rssitem.h"
#include "utf8string.h"

namespace newsboat {

typedef std::pair<Utf8String, std::shared_ptr<Matcher>> FeedUrlExprPair;

class RssIgnores : public ConfigActionHandler {
public:
	RssIgnores() {}
	~RssIgnores() override {}
	void handle_action(const Utf8String& action,
		const std::vector<Utf8String>& params) override;
	void dump_config(std::vector<Utf8String>& config_output) const override;
	bool matches(RssItem* item);
	bool matches_lastmodified(const Utf8String& url);
	bool matches_resetunread(const Utf8String& url);

private:
	std::vector<FeedUrlExprPair> ignores;
	std::vector<Utf8String> ignores_lastmodified;
	std::vector<Utf8String> resetflag;

	static const Utf8String REGEX_PREFIX;
};

} // namespace newsboat

#endif /* NEWSBOAT_RSSIGNORES_H_ */
