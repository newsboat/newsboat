#include <filtercontainer.h>

namespace newsbeuter {

filtercontainer::~filtercontainer() { }

action_handler_status filtercontainer::handle_action(const std::string& action, const std::vector<std::string>& params) {
	/*
	 * filtercontainer does nothing but to save (filter name, filter expression) tuples.
	 * These tuples are used for enabling the user to predefine filter expressions and
	 * then select them from a list by their name.
	 */
	if (action == "define-filter") {
		if (params.size() >= 2) {
			filters.push_back(filter_name_expr_pair(params[0],params[1]));
			return AHS_OK;
		} else {
			return AHS_TOO_FEW_PARAMS;
		}
	}
	return AHS_INVALID_COMMAND;
}

}
