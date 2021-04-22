#ifndef NEWSBOAT_FILTERCONTAINER_H_
#define NEWSBOAT_FILTERCONTAINER_H_

#include "configactionhandler.h"
#include "utf8string.h"

namespace newsboat {

/// Stores the name of the filter and the filter expression that user added via
/// `define-filter` configuration option
struct FilterNameExprPair {
	/// Name of the filter
	std::string name;

	/// Filter expression for this named filter
	std::string expr;
};

/// Stores the name of the filter and the filter expression that user added via
/// `define-filter` configuration option
struct InternalFilterNameExprPair {
	/// Name of the filter
	Utf8String name;

	/// Filter expression for this named filter
	Utf8String expr;
};

class FilterContainer : public ConfigActionHandler {
public:
	FilterContainer() = default;
	~FilterContainer() override;
	void handle_action(const std::string& action,
		const std::vector<std::string>& params) override;
	void dump_config(std::vector<std::string>& config_output) const override;
	// FIXME(utf8): change this back to const std::vector<>&
	std::vector<FilterNameExprPair> get_filters() const
	{
		std::vector<FilterNameExprPair> result;
		for (const auto& filter : filters) {
			FilterNameExprPair entry;
			entry.name = filter.name.to_utf8();
			entry.expr = filter.expr.to_utf8();
			result.push_back(std::move(entry));
		}
		return result;
	}
	unsigned int size()
	{
		return filters.size();
	}

private:
	std::vector<InternalFilterNameExprPair> filters;
};

} // namespace newsboat

#endif /* NEWSBOAT_FILTERCONTAINER_H_ */
