#include "filtercontainer.h"

#include <algorithm>

#include "config.h"
#include "confighandlerexception.h"
#include "configparser.h"
#include "matcher.h"
#include "strprintf.h"
#include "utils.h"

namespace Newsboat {

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
		if (params.size() < 2) {
			throw ConfigHandlerException(ActionHandlerStatus::TOO_FEW_PARAMS);
		}

		FilterNameExprPair filter;
		filter.name = params[0];
		filter.expr = params[1];

		Matcher m;
		if (!m.parse(filter.expr)) {
			throw ConfigHandlerException(strprintf::fmt(
					_("couldn't parse filter expression `%s': %s"),
					filter.expr,
					m.get_parse_error()));
		}

		filters.emplace_back(std::move(filter));
	} else {
		throw ConfigHandlerException(ActionHandlerStatus::INVALID_COMMAND);
	}
}

void FilterContainer::dump_config(std::vector<std::string>& config_output) const
{
	for (const auto& filter : filters) {
		config_output.push_back(strprintf::fmt("define-filter %s %s",
				utils::quote(filter.name),
				utils::quote(filter.expr)));
	}
}

std::optional<std::string> FilterContainer::get_filter(const std::string& name)
{
	const auto filter = std::find_if(filters.begin(),
	filters.end(), [&](const FilterNameExprPair& pair) {
		return pair.name == name;
	});

	if (filter != filters.end()) {
		return filter->expr;
	} else {
		return {};
	}
}

} // namespace Newsboat
