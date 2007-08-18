#ifndef NEWSBEUTER_FILTERCONTAINER__H
#define NEWSBEUTER_FILTERCONTAINER__H

#include <configparser.h>

namespace newsbeuter {

class filtercontainer : public config_action_handler {
	public:
		virtual ~filtercontainer();
		virtual action_handler_status handle_action(const std::string& action, const std::vector<std::string>& params);
		inline std::vector<std::pair<std::string,std::string> >& get_filters() { return filters; }
		inline unsigned int size() { return filters.size(); }
	private:
		std::vector<std::pair<std::string,std::string> > filters;

};


}


#endif
