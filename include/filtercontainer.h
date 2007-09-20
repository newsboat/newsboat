#ifndef NEWSBEUTER_FILTERCONTAINER__H
#define NEWSBEUTER_FILTERCONTAINER__H

#include <configparser.h>

namespace newsbeuter {

typedef std::pair<std::string, std::string> filter_name_expr_pair;

class filtercontainer : public config_action_handler {
	public:
		virtual ~filtercontainer();
		virtual action_handler_status handle_action(const std::string& action, const std::vector<std::string>& params);
		inline std::vector<filter_name_expr_pair>& get_filters() { return filters; }
		inline unsigned int size() { return filters.size(); }
	private:
		std::vector<filter_name_expr_pair> filters;

};


}


#endif
