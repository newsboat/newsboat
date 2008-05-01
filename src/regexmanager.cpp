#include <regexmanager.h>
#include <logger.h>
#include <utils.h>
#include <cstring>

namespace newsbeuter {

regexmanager::regexmanager() {

}

regexmanager::~regexmanager() {
	for (std::vector<regex_t *>::iterator it=regexes.begin();it!=regexes.end();++it) {
		delete *it;
	}
}

action_handler_status regexmanager::handle_action(const std::string& action, const std::vector<std::string>& params) {
	if (action == "highlight-pattern") {
		if (params.size() < 2)
			return AHS_TOO_FEW_PARAMS;
		regex_t * rx = new regex_t;
		if (regcomp(rx, params[0].c_str(), REG_EXTENDED | REG_ICASE) != 0) {
			delete rx;
			return AHS_INVALID_PARAMS;
		}
		std::string colorstr;
		if (params[1] != "default") {
			colorstr.append("fg=");
			colorstr.append(params[1]);
		}
		if (params.size() > 2) {
			if (params[2] != "default") {
				if (colorstr.length() > 0)
					colorstr.append(",");
				colorstr.append("bg=");
				colorstr.append(params[2]);
			}
			for (unsigned int i=3;i<params.size();++i) {
				if (params[i] != "default") {
					if (colorstr.length() > 0)
						colorstr.append(",");
					colorstr.append("attr=");
					colorstr.append(params[i]);
				}
			}
		}
		GetLogger().log(LOG_DEBUG, "regexmanager::handle_action: adding rx = %s colorstr = %s", params[0].c_str(), colorstr.c_str());
		regexes.push_back(rx);
		colors.push_back(colorstr);
		return AHS_OK;
	} else
		return AHS_INVALID_COMMAND;
}

void regexmanager::quote_and_highlight(std::string& str) {
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
