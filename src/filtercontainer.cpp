#include <filtercontainer.h>

namespace newsbeuter {

filtercontainer::~filtercontainer() { }

action_handler_status filtercontainer::handle_action(const std::string& action, const std::vector<std::string>& params) {
	if (action == "define-filter") {
		if (params.size() >= 2) {
			filters.push_back(std::pair<std::string,std::string>(params[0],params[1]));
			return AHS_OK;
		} else {
			return AHS_TOO_FEW_PARAMS;
		}
	}
	return AHS_INVALID_COMMAND;
}

}
