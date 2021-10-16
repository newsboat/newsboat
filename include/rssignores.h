#ifndef NEWSBOAT_RSSIGNORES_H_
#define NEWSBOAT_RSSIGNORES_H_

#include <string>
#include <vector>

#include "configactionhandler.h"
#include "matcher.h"
#include "rssitem.h"

namespace newsboat {

typedef std::pair<std::string, std::shared_ptr<Matcher>> FeedUrlExprPair;

class RssIgnores : public ConfigActionHandler {
public:
	RssIgnores() {}
	~RssIgnores() override {}
	void handle_action(const std::string& action,
		const std::vector<std::string>& params) override;
	void dump_config(std::vector<std::string>& config_output) const override;
	bool matches(RssItem* item);
	bool matches_lastmodified(const std::string& url);
	bool matches_resetunread(const std::string& url);

private:
	std::vector<FeedUrlExprPair> ignores;
	std::vector<std::string> ignores_lastmodified;
	std::vector<std::string> resetflag;
};

} // namespace newsboat

#endif /* NEWSBOAT_RSSIGNORES_H_ */
