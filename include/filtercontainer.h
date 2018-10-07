#ifndef NEWSBOAT_FILTERCONTAINER_H_
#define NEWSBOAT_FILTERCONTAINER_H_

#include "configparser.h"

namespace newsboat {

typedef std::pair<std::string, std::string> FilterNameExprPair;

class FilterContainer : public ConfigActionHandler {
public:
	FilterContainer() {}
	~FilterContainer() override;
	void handle_action(const std::string& action,
		const std::vector<std::string>& params) override;
	void dump_config(std::vector<std::string>& config_output) override;
	std::vector<FilterNameExprPair>& get_filters()
	{
		return filters;
	}
	unsigned int size()
	{
		return filters.size();
	}

private:
	std::vector<FilterNameExprPair> filters;
};

} // namespace newsboat

#endif /* NEWSBOAT_FILTERCONTAINER_H_ */
