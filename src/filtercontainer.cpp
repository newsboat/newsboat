#include "filtercontainer.h"

#include "config.h"
#include "confighandlerexception.h"
#include "configparser.h"
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
		if (params.size() < 2) {
			throw ConfigHandlerException(ActionHandlerStatus::TOO_FEW_PARAMS);
		}

		InternalFilterNameExprPair filter;
		filter.name = Utf8String::from_utf8(params[0]);
		filter.expr = Utf8String::from_utf8(params[1]);

		Matcher m;
		if (!m.parse(filter.expr.to_utf8())) {
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
				utils::quote(filter.name.to_utf8()),
				utils::quote(filter.expr.to_utf8())));
	}
}

} // namespace newsboat
