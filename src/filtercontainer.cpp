#include <filtercontainer.h>
#include <exceptions.h>
#include <matcher.h>
#include <utils.h>
#include <config.h>

namespace newsbeuter {

filtercontainer::~filtercontainer() { }

void filtercontainer::handle_action(const std::string& action, const std::vector<std::string>& params) {
	/*
	 * filtercontainer does nothing but to save (filter name, filter expression) tuples.
	 * These tuples are used for enabling the user to predefine filter expressions and
	 * then select them from a list by their name.
	 */
	if (action == "define-filter") {
		if (params.size() < 2)
			throw confighandlerexception(AHS_TOO_FEW_PARAMS);
		matcher m;
		if (!m.parse(params[1]))
			throw confighandlerexception(utils::strprintf(_("couldn't parse filter expression `%s': %s"), params[1].c_str(), m.get_parse_error().c_str()));
		filters.push_back(filter_name_expr_pair(params[0],params[1]));
	} else
		throw confighandlerexception(AHS_INVALID_COMMAND);
}

void filtercontainer::dump_config(std::vector<std::string>& config_output) {
	for (auto filter : filters) {
		config_output.push_back(utils::strprintf("define-filter %s %s", utils::quote(filter.first).c_str(), utils::quote(filter.second).c_str()));
	}
}

}
