#ifndef NEWSBOAT_FILTERCONTAINER_H_
#define NEWSBOAT_FILTERCONTAINER_H_

#include "configactionhandler.h"
#include "utf8string.h"

#include "3rd-party/optional.hpp"

namespace newsboat {

/// Stores the name of the filter and the filter expression that user added via
/// `define-filter` configuration option
struct FilterNameExprPair {
	/// Name of the filter
	Utf8String name;

	/// Filter expression for this named filter
	Utf8String expr;
};

class FilterContainer : public ConfigActionHandler {
public:
	FilterContainer() = default;
	~FilterContainer() override;
	void handle_action(const Utf8String& action,
		const std::vector<Utf8String>& params) override;
	void dump_config(std::vector<Utf8String>& config_output) const override;
	const std::vector<FilterNameExprPair>& get_filters() const
	{
		return filters;
	}
	unsigned int size()
	{
		return filters.size();
	}

	nonstd::optional<Utf8String> get_filter(const Utf8String& name);

private:
	std::vector<FilterNameExprPair> filters;
};

} // namespace newsboat

#endif /* NEWSBOAT_FILTERCONTAINER_H_ */
