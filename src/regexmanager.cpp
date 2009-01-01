#include <regexmanager.h>
#include <logger.h>
#include <utils.h>
#include <cstring>

namespace newsbeuter {

regexmanager::regexmanager() {
	// this creates the entries in the map. we need them there to have the "all" location work.
	locations["article"];
	locations["articlelist"];
	locations["feedlist"];
}

regexmanager::~regexmanager() {
	for (std::map<std::string, rc_pair>::iterator jt=locations.begin();jt!=locations.end();jt++) {
		std::vector<regex_t *>& regexes(jt->second.first);
		if (regexes.size() > 0) {
			for (std::vector<regex_t *>::iterator it=regexes.begin();it!=regexes.end();++it) {
				delete *it;
			}
		}
	}
}

action_handler_status regexmanager::handle_action(const std::string& action, const std::vector<std::string>& params) {
	if (action == "highlight") {
		if (params.size() < 3)
			return AHS_TOO_FEW_PARAMS;

		std::string location = params[0];
		if (location != "all" && location != "article" && location != "articlelist" && location != "feedlist")
			return AHS_INVALID_PARAMS;

		regex_t * rx = new regex_t;
		if (regcomp(rx, params[1].c_str(), REG_EXTENDED | REG_ICASE) != 0) {
			delete rx;
			return AHS_INVALID_PARAMS;
		}
		std::string colorstr;
		if (params[2] != "default") {
			colorstr.append("fg=");
			if (!utils::is_valid_color(params[2]))
				return AHS_INVALID_PARAMS;
			colorstr.append(params[2]);
		}
		if (params.size() > 2) {
			if (params[3] != "default") {
				if (colorstr.length() > 0)
					colorstr.append(",");
				colorstr.append("bg=");
				if (!utils::is_valid_color(params[3]))
					return AHS_INVALID_PARAMS;
				colorstr.append(params[3]);
			}
			for (unsigned int i=4;i<params.size();++i) {
				if (params[i] != "default") {
					if (colorstr.length() > 0)
						colorstr.append(",");
					colorstr.append("attr=");
					if (!utils::is_valid_attribute(params[i]))
						return AHS_INVALID_PARAMS;
					colorstr.append(params[i]);
				}
			}
		}
		if (location != "all") {
			GetLogger().log(LOG_DEBUG, "regexmanager::handle_action: adding rx = %s colorstr = %s to location %s",
				params[1].c_str(), colorstr.c_str(), location.c_str());
			locations[location].first.push_back(rx);
			locations[location].second.push_back(colorstr);
		} else {
			delete rx;
			for (std::map<std::string, rc_pair>::iterator it=locations.begin();it!=locations.end();it++) {
				GetLogger().log(LOG_DEBUG, "regexmanager::handle_action: adding rx = %s colorstr = %s to location %s",
					params[1].c_str(), colorstr.c_str(), it->first.c_str());
				rx = new regex_t;
 				// we need to create a new one for each push_back, otherwise we'd have double frees.
				regcomp(rx, params[1].c_str(), REG_EXTENDED | REG_ICASE);
				it->second.first.push_back(rx);
				it->second.second.push_back(colorstr);
			}
		}
		return AHS_OK;
	} else
		return AHS_INVALID_COMMAND;
}

void regexmanager::remove_last_regex(const std::string& location) {
	std::vector<regex_t *>& regexes = locations[location].first;

	std::vector<regex_t *>::iterator it=regexes.begin();
	for (unsigned int i=0;i<regexes.size()-1;i++) {
		it++;
	}
	delete *it;
	regexes.erase(it);
}



void regexmanager::quote_and_highlight(std::string& str, const std::string& location) {
	std::vector<regex_t *>& regexes = locations[location].first;

	unsigned int len = str.length();
	for (unsigned int i=0;i<len;++i) {
		if (str[i] == '<') {
			str.insert(i+1, ">");
			++len;
		}
	}
	unsigned int i = 0;
	for (std::vector<regex_t *>::iterator it=regexes.begin();it!=regexes.end();++it, ++i) {
		regmatch_t pmatch;
		unsigned int offset = 0;
		int err = regexec(*it, str.c_str(), 1, &pmatch, 0);
		while (err == 0) {
			// GetLogger().log(LOG_DEBUG, "regexmanager::quote_and_highlight: matched %s rm_so = %u rm_eo = %u", str.c_str() + offset, pmatch.rm_so, pmatch.rm_eo);
			std::string marker = utils::strprintf("<%u>", i);
			str.insert(offset + pmatch.rm_eo, "</>");
			// GetLogger().log(LOG_DEBUG, "after first insert: %s", str.c_str());
			str.insert(offset + pmatch.rm_so, marker);
			// GetLogger().log(LOG_DEBUG, "after second insert: %s", str.c_str());
			offset += pmatch.rm_eo + marker.length() + strlen("</>");
			err = regexec(*it, str.c_str() + offset, 1, &pmatch, 0);
		}
	}
}

}
