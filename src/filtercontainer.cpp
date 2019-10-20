#include "filtercontainer.h"

#include "config.h"
#include "confighandlerexception.h"
#include "matcher.h"
#include "strprintf.h"
#include "utils.h"

namespace newsboat {

FilterContainer::~FilterContainer() {}

void FilterContainer::handle_action(const std::string& action,
	const std::vector<std::string>& params)
{
	/*
	 * FilterContainer does nothing but to save (filter name, filter
	 * expression) tuples. These tuples are used for enabling the user to
	 * predefine filter expressions and then select them from a list by
	 * their name.
	 */
	if (action == "define-filter") {
		if (params.size() < 2)
			throw ConfigHandlerException(
				ActionHandlerStatus::TOO_FEW_PARAMS);
		Matcher m;
		if (!m.parse(params[1]))
			throw ConfigHandlerException(strprintf::fmt(
					_("couldn't parse filter expression `%s': %s"),
					params[1],
					m.get_parse_error()));
		filters.push_back(FilterNameExprPair(params[0], params[1]));
	} else
		throw ConfigHandlerException(
			ActionHandlerStatus::INVALID_COMMAND);
}

void FilterContainer::dump_config(std::vector<std::string>& config_output)
{
	for (const auto& filter : filters) {
		config_output.push_back(strprintf::fmt("define-filter %s %s",
				utils::quote(filter.first),
				utils::quote(filter.second)));
	}
}

} // namespace newsboat
