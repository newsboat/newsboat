#ifndef NEWSBOAT_RSSIGNORES_H_
#define NEWSBOAT_RSSIGNORES_H_

#include <string>
#include <unordered_map>
#include <vector>

#include "configactionhandler.h"
#include "matcher.h"
#include "rssitem.h"

namespace newsboat {

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
	bool matches_expr(std::shared_ptr<Matcher> expr, RssItem* item);

	std::unordered_multimap<std::string, std::shared_ptr<Matcher>> ignores;
	std::vector<std::string> ignores_lastmodified;
	std::vector<std::string> resetflag;

	static const std::string REGEX_PREFIX;
};

} // namespace newsboat

#endif /* NEWSBOAT_RSSIGNORES_H_ */
