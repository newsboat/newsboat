#include <fstream>
#include <cstring>

#include <urlreader.h>
#include <utils.h>
#include <logger.h>
#include <sys/utsname.h>
#include <config.h>

namespace newsbeuter {

urlreader::urlreader() : offline(false) { }
urlreader::~urlreader() { }

std::vector<std::string>& urlreader::get_urls() {
	return urls;
}

std::vector<std::string>& urlreader::get_tags(const std::string& url) {
	return tags[url];
}

std::vector<std::string> urlreader::get_alltags() {
	std::vector<std::string> tmptags;
	for (auto t : alltags) {
		if (t.substr(0,1) != "~")
			tmptags.push_back(t);
	}
	return tmptags;
}


file_urlreader::file_urlreader(const std::string& file) : filename(file) { }
file_urlreader::~file_urlreader() { }

std::string file_urlreader::get_source() {
	return filename;
}

void file_urlreader::reload() {
	if (offline)
		return;

	urls.clear();
	tags.clear();
	alltags.clear();

	std::fstream f;
	f.open(filename.c_str(),std::fstream::in);
	if (f.is_open()) {
		std::string line;
		do {
			getline(f,line);
			if (!f.eof() && line.length() > 0 && line[0] != '#') {
				std::vector<std::string> tokens = utils::tokenize_quoted(line);
				if (!tokens.empty()) {
					std::string url = tokens[0];
					urls.push_back(url);
					tokens.erase(tokens.begin());
					if (!tokens.empty()) {
						tags[url] = tokens;
						for (auto token : tokens) {
							alltags.insert(token);
						}
					}
				}
			}
		} while (!f.eof());
	}
}

void file_urlreader::load_config(const std::string& file) {
	filename = file;
	reload();
}

void file_urlreader::write_config() {
	std::fstream f;
	f.open(filename.c_str(),std::fstream::out);
	if (f.is_open()) {
		for (auto url : urls) {
			f << url;
			if (tags[url].size() > 0) {
				for (auto tag : tags[url]) {
					f << " \"" << tag << "\"";
				}
			}
			f << std::endl;
		}
	}
}


opml_urlreader::opml_urlreader(configcontainer * c) : cfg(c) { }
opml_urlreader::~opml_urlreader() { }


void opml_urlreader::write_config() {
	// do nothing.
}


void opml_urlreader::reload() {
	if (offline)
		return;

	urls.clear();
	tags.clear();
	alltags.clear();

	std::string user_agent = utils::get_useragent(cfg);

	std::vector<std::string> urls = utils::tokenize_quoted(this->get_source(), " ");

	for (auto url : urls) {
		LOG(LOG_DEBUG, "opml_urlreader::reload: downloading `%s'", url.c_str());
		std::string urlcontent = utils::retrieve_url(url, cfg, this->get_auth());

		xmlDoc * doc = xmlParseMemory(urlcontent.c_str(), urlcontent.length());

		if (doc == NULL) {
			LOG(LOG_ERROR, "opml_urlreader::reload: parsing XML file failed");
			continue;
		}

		xmlNode * root = xmlDocGetRootElement(doc);

		if (root) {
			for (xmlNode * node = root->children; node != NULL; node = node->next) {
				if (strcmp((const char *)node->name, "body")==0) {
					LOG(LOG_DEBUG, "opml_urlreader::reload: found body");
					rec_find_rss_outlines(node->children, "");
				}
			}
		}

		xmlFreeDoc(doc);
	}
}

void opml_urlreader::handle_node(xmlNode * node, const std::string& tag) {
	if (node) {
		char * rssurl = (char *)xmlGetProp(node, (const xmlChar *)"xmlUrl");
		if (rssurl && strlen(rssurl) > 0) {
			std::string theurl(rssurl);
			urls.push_back(theurl);
			if (tag.length() > 0) {
				std::vector<std::string> tmptags;
				tmptags.push_back(tag);
				tags[theurl] = tmptags;
				alltags.insert(tag);
			}
		}
		if (rssurl)
			xmlFree(rssurl);
	}
}

void opml_urlreader::rec_find_rss_outlines(xmlNode * node, std::string tag) {
	while (node) {
		char * type = (char *)xmlGetProp(node, (const xmlChar *)"type");

		std::string newtag = tag;

		if (strcmp((const char *)node->name, "outline")==0) {
			 if (type && strcmp(type,"rss")==0) {
				 handle_node(node, tag);
			  } else {
				char * text = (char *)xmlGetProp(node, (const xmlChar *)"title");
				if (text) {
					if (newtag.length() > 0) {
						newtag.append("/");
					}
					newtag.append(text);
					xmlFree(text);
				}
			}
		}
		rec_find_rss_outlines(node->children, newtag);
		node = node->next;
	}
}


std::string opml_urlreader::get_source() {
	return cfg->get_configvalue("opml-url");
}

const char * opml_urlreader::get_auth() {
	return NULL;
}

}
