#include <regexmanager.h>
#include <logger.h>
#include <utils.h>
#include <cstring>
#include <exceptions.h>
#include <config.h>

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

void regexmanager::dump_config(std::vector<std::string>& config_output) {
	for (std::vector<std::string>::iterator it=cheat_store_for_dump_config.begin();it!=cheat_store_for_dump_config.end();it++) {
		config_output.push_back(*it);
	}
}

void regexmanager::handle_action(const std::string& action, const std::vector<std::string>& params) {
	if (action == "highlight") {
		if (params.size() < 3)
			throw confighandlerexception(AHS_TOO_FEW_PARAMS);

		std::string location = params[0];
		if (location != "all" && location != "article" && location != "articlelist" && location != "feedlist")
			throw confighandlerexception(utils::strprintf(_("`%s' is an invalid dialog type"), location.c_str()));

		regex_t * rx = new regex_t;
		int err;
		if ((err = regcomp(rx, params[1].c_str(), REG_EXTENDED | REG_ICASE)) != 0) {
			delete rx;
			char buf[1024];
			regerror(err, rx, buf, sizeof(buf));
			throw confighandlerexception(utils::strprintf(_("`%s' is not a valid regular expression: %s"), params[1].c_str(), buf));
		}
		std::string colorstr;
		if (params[2] != "default") {
			colorstr.append("fg=");
			if (!utils::is_valid_color(params[2]))
				throw confighandlerexception(utils::strprintf(_("`%s' is not a valid color"), params[2].c_str()));
			colorstr.append(params[2]);
		}
		if (params.size() > 2) {
			if (params[3] != "default") {
				if (colorstr.length() > 0)
					colorstr.append(",");
				colorstr.append("bg=");
				if (!utils::is_valid_color(params[3]))
					throw confighandlerexception(utils::strprintf(_("`%s' is not a valid color"), params[3].c_str()));
				colorstr.append(params[3]);
			}
			for (unsigned int i=4;i<params.size();++i) {
				if (params[i] != "default") {
					if (colorstr.length() > 0)
						colorstr.append(",");
					colorstr.append("attr=");
					if (!utils::is_valid_attribute(params[i]))
						throw confighandlerexception(utils::strprintf(_("`%s' is not a valid attribute"), params[i].c_str()));
					colorstr.append(params[i]);
				}
			}
		}
		if (location != "all") {
			LOG(LOG_DEBUG, "regexmanager::handle_action: adding rx = %s colorstr = %s to location %s",
				params[1].c_str(), colorstr.c_str(), location.c_str());
			locations[location].first.push_back(rx);
			locations[location].second.push_back(colorstr);
		} else {
			delete rx;
			for (std::map<std::string, rc_pair>::iterator it=locations.begin();it!=locations.end();it++) {
				LOG(LOG_DEBUG, "regexmanager::handle_action: adding rx = %s colorstr = %s to location %s",
					params[1].c_str(), colorstr.c_str(), it->first.c_str());
				rx = new regex_t;
 				// we need to create a new one for each push_back, otherwise we'd have double frees.
				regcomp(rx, params[1].c_str(), REG_EXTENDED | REG_ICASE);
				it->second.first.push_back(rx);
				it->second.second.push_back(colorstr);
			}
		}
		std::string line = "highlight";
		for (std::vector<std::string>::const_iterator it=params.begin();it!=params.end();it++) {
			line.append(" ");
			line.append(utils::quote(*it));
		}
		cheat_store_for_dump_config.push_back(line);
	} else if (action == "highlight-article") {
		if (params.size() < 3)
			throw confighandlerexception(AHS_TOO_FEW_PARAMS);

		std::string expr = params[0];
		std::string fgcolor = params[1];
		std::string bgcolor = params[2];

		std::string colorstr;
		if (fgcolor != "default") {
			colorstr.append("fg=");
			if (!utils::is_valid_color(fgcolor))
				throw confighandlerexception(utils::strprintf(_("`%s' is not a valid color"), fgcolor.c_str()));
			colorstr.append(fgcolor);
		}
		if (bgcolor != "default") {
			if (colorstr.length() > 0)
				colorstr.append(",");
			colorstr.append("bg=");
			if (!utils::is_valid_color(bgcolor))
				throw confighandlerexception(utils::strprintf(_("`%s' is not a valid color"), bgcolor.c_str()));
			colorstr.append(bgcolor);
		}

		for (unsigned int i=3;i<params.size();i++) {
			if (params[i] != "default") {
				if (colorstr.length() > 0)
					colorstr.append(",");
				colorstr.append("attr=");
				if (!utils::is_valid_attribute(params[i]))
					throw confighandlerexception(utils::strprintf(_("`%s' is not a valid attribute"), params[i].c_str()));
				colorstr.append(params[i]);
			}
		}

		std::tr1::shared_ptr<matcher> m(new matcher());
		if (!m->parse(params[0])) {
			throw confighandlerexception(utils::strprintf(_("couldn't parse filter expression `%s': %s"), params[0].c_str(), m->get_parse_error().c_str()));
		}

		int pos = locations["articlelist"].first.size();

		locations["articlelist"].first.push_back(NULL);
		locations["articlelist"].second.push_back(colorstr);

		matchers.push_back(std::pair<std::tr1::shared_ptr<matcher>, int>(m, pos));

	} else
		throw confighandlerexception(AHS_INVALID_COMMAND);
}

int regexmanager::article_matches(matchable * item) {
	for (std::vector<std::pair<std::tr1::shared_ptr<matcher>, int> >::iterator it=matchers.begin();it!=matchers.end();it++) {
		if (it->first->matches(item)) {
			return it->second;
		}
	}
	return -1;
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

std::string regexmanager::extract_initial_marker(const std::string& str) {
	if (str.length() == 0)
		return "";

	if (str[0] == '<') {
		std::string::size_type pos = str.find_first_of(">", 0);
		if (pos != std::string::npos) {
			return str.substr(0, pos+1);
		}
	}
	return "";
}


void regexmanager::quote_and_highlight(std::string& str, const std::string& location) {
	std::vector<regex_t *>& regexes = locations[location].first;

	unsigned int i = 0;
	for (std::vector<regex_t *>::iterator it=regexes.begin();it!=regexes.end();++it, ++i) {
		if (!*it)
			continue;
		std::string initial_marker = extract_initial_marker(str);
		regmatch_t pmatch;
		unsigned int offset = 0;
		int err = regexec(*it, str.c_str(), 1, &pmatch, 0);
		while (err == 0) {
			// LOG(LOG_DEBUG, "regexmanager::quote_and_highlight: matched %s rm_so = %u rm_eo = %u", str.c_str() + offset, pmatch.rm_so, pmatch.rm_eo);
			std::string marker = utils::strprintf("<%u>", i);
			str.insert(offset + pmatch.rm_eo, std::string("</>") + initial_marker);
			// LOG(LOG_DEBUG, "after first insert: %s", str.c_str());
			str.insert(offset + pmatch.rm_so, marker);
			// LOG(LOG_DEBUG, "after second insert: %s", str.c_str());
			offset += pmatch.rm_eo + marker.length() + strlen("</>") + initial_marker.length();
			err = regexec(*it, str.c_str() + offset, 1, &pmatch, 0);
		}
	}
}

}
