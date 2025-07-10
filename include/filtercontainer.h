#ifndef NEWSBOAT_FILTERCONTAINER_H_
#define NEWSBOAT_FILTERCONTAINER_H_

#include <optional>

#include "configactionhandler.h"

namespace Newsboat {

/// Stores the name of the filter and the filter expression that user added via
/// `define-filter` configuration option
struct FilterNameExprPair {
	/// Name of the filter
	std::string name;

	/// Filter expression for this named filter
	std::string expr;
};

class FilterContainer : public ConfigActionHandler {
public:
	FilterContainer() = default;
	~FilterContainer() override;
	void handle_action(const std::string& action,
		const std::vector<std::string>& params) override;
	void dump_config(std::vector<std::string>& config_output) const override;
	std::vector<FilterNameExprPair>& get_filters()
	{
		return filters;
	}
	unsigned int size()
	{
		return filters.size();
	}

	std::optional<std::string> get_filter(const std::string& name);

private:
	std::vector<FilterNameExprPair> filters;
};

} // namespace Newsboat

#endif /* NEWSBOAT_FILTERCONTAINER_H_ */
