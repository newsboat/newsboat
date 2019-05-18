#ifndef NEWSBOAT_RSS_H_
#define NEWSBOAT_RSS_H_

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "configcontainer.h"
#include "matchable.h"
#include "matcher.h"
#include "rssitem.h"
#include "utils.h"

namespace newsboat {

typedef std::pair<std::string, Matcher*> FeedUrlExprPair;

class Cache;

class RssIgnores : public ConfigActionHandler {
public:
	RssIgnores() {}
	~RssIgnores() override;
	void handle_action(const std::string& action,
		const std::vector<std::string>& params) override;
	void dump_config(std::vector<std::string>& config_output) override;
	bool matches(RssItem* item);
	bool matches_lastmodified(const std::string& url);
	bool matches_resetunread(const std::string& url);

private:
	std::vector<FeedUrlExprPair> ignores;
	std::vector<std::string> ignores_lastmodified;
	std::vector<std::string> resetflag;
};

} // namespace newsboat

#endif /* NEWSBOAT_RSS_H_ */
