#include <htmltmpl.h>
#include <logger.h>
#include <utils.h>
#include <sstream>

using namespace newsbeuter;

namespace planet {

	htmltmpl::htmltmpl() : type(NT_ROOT) { }

	htmltmpl::~htmltmpl() { }

	htmltmpl htmltmpl::parse(const std::string& str) {
		std::istringstream is(str);
		return parse_stream(is);
	}

	htmltmpl htmltmpl::parse_stream(std::istream& is, const std::string& endtag) {
		htmltmpl node;
		while (!is.eof()) {
			std::string curelem;
			getline(is, curelem, '#');

			if (!is.eof() && curelem.length() > 0) {
				htmltmpl textnode;
				textnode.type = NT_TEXT;
				textnode.text = curelem;
				GetLogger().log(LOG_DEBUG, "htmltmpl::parse_stream: NT_TEXT: %s", curelem.c_str());
				node.children.push_back(textnode);
			}

			if (!is.eof()) {
				getline(is, curelem, '#');

				// if we found the optional end tag, then break
				if (endtag.length() > 0 && curelem == endtag) {
					GetLogger().log(LOG_DEBUG, "htmltmpl::parse_stream: found endtag: %s", curelem.c_str());
					break;
				}

				if (curelem.substr(0,4) == "LOOP") {
					std::vector<std::string> loopelem = utils::tokenize(curelem," ");
					if (loopelem.size() == 2) {
						GetLogger().log(LOG_DEBUG, "htmltmpl::parse_stream: before NT_LOOP: %s", loopelem[1].c_str());
						htmltmpl loopnode = parse_stream(is, "ENDLOOP");
						GetLogger().log(LOG_DEBUG, "htmltmpl::parse_stream: after NT_LOOP: %s", loopelem[1].c_str());
						loopnode.type = NT_LOOP;
						loopnode.text = loopelem[1];
						node.children.push_back(loopnode);
					}
				} else if (curelem.substr(0,2) == "IF") {
					std::vector<std::string> ifelem = utils::tokenize(curelem," ");
					if (ifelem.size() == 2) {
						GetLogger().log(LOG_DEBUG, "htmltmpl::parse_stream: before NT_IF: %s", ifelem[1].c_str());
						htmltmpl ifnode = parse_stream(is, "ENDIF");
						GetLogger().log(LOG_DEBUG, "htmltmpl::parse_stream: after NT_IF: %s", ifelem[1].c_str());
						ifnode.type = NT_IF;
						ifnode.text = ifelem[1];
						node.children.push_back(ifnode);
					}
				} else if (curelem.substr(0,3) == "VAR") {
					std::vector<std::string> var = utils::tokenize(curelem," ");
					if (var.size() == 2) {
						htmltmpl varnode;
						varnode.type = NT_VAR;
						varnode.text = var[1];
						GetLogger().log(LOG_DEBUG, "htmltmpl::parse_stream: NT_VAR: %s", var[1].c_str());
						node.children.push_back(varnode);
					}
				} else {
					GetLogger().log(LOG_WARN, "htmltmpl::parse_stream: unknown command %s", curelem.c_str());
				}
			}
		}
		return node;
	}

	htmltmpl htmltmpl::parse_file(const std::string& file) {
		std::fstream f(file.c_str(), std::fstream::in);
		if (f.is_open()) {
			return parse_stream(f);
		}
		return htmltmpl();
	}

	std::string htmltmpl::to_html(const std::vector<rss_feed>& feeds, const std::vector<rss_item>& items, varmap variables) {
		std::string result;
		switch (type) {
			case NT_ROOT:
				for (std::vector<htmltmpl>::iterator it=children.begin();it!=children.end();++it) {
					result.append(it->to_html(feeds, items, variables));
				}
				break;
			case NT_TEXT:
				result.append(text);
				break;
			case NT_LOOP:
				if (text == "Feeds") {
					for (std::vector<rss_feed>::const_iterator it=feeds.begin();it!=feeds.end();++it) {
						variables["feed_title"] = it->title();
						variables["feed_desc"] = it->description();
						variables["feed_link"] = it->link();
						variables["feed_rssurl"] = it->rssurl();
						// variables["feed_tags"] = it->get_tags(); // that will change, I guess...
						for (std::vector<htmltmpl>::iterator jt=children.begin();jt!=children.end();++jt) {
							result.append(jt->to_html(feeds, items, variables));
						}
					}
				} else if (text == "Items") {
					for (std::vector<rss_item>::const_iterator it=items.begin();it!=items.end();++it) {
						variables["item_title"] = it->title();
						variables["item_body"] = it->description();
						variables["item_link"] = it->link();
						variables["item_author"] = it->author();
						variables["item_date"] = it->pubDate();
						variables["item_enclosure_url"] = it->enclosure_url();
						// variables["feed_tags"] = it->get_tags(); // that will change, I guess...
						for (std::vector<htmltmpl>::iterator jt=children.begin();jt!=children.end();++jt) {
							result.append(jt->to_html(feeds, items, variables));
						}
					}
				} else {
					GetLogger().log(LOG_WARN,"htmltmpl::to_html: unknown loop type %s", text.c_str());
				}
				break;
			case NT_IF:
				if (variables[text].length() > 0) {
					for (std::vector<htmltmpl>::iterator it=children.begin();it!=children.end();++it) {
						result.append(it->to_html(feeds, items, variables));
					}
				}
				break;
			case NT_VAR:
				result.append(variables[text]);
				break;
			default:
				GetLogger().log(LOG_WARN,"htmltmpl::to_html: unknown node type %u", type);
				break;
		}
		return result;
	}




}
