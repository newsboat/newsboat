#ifndef NEWSBEUTER_CONFIGREADER__H
#define NEWSBEUTER_CONFIGREADER__H

#include <vector>
#include <map>
#include <set>
#include <string>

#include <configcontainer.h>

#include <libxml/tree.h>

namespace newsbeuter {

	class urlreader {
		public:
			urlreader();
			virtual ~urlreader();
			virtual void write_config() = 0;
			virtual void reload() = 0;
			virtual std::string get_source() = 0;
			std::vector<std::string>& get_urls();
			std::vector<std::string>& get_tags(const std::string& url);
			std::vector<std::string> get_alltags();
			inline void set_offline(bool off) { offline = off; }
		protected:
			std::vector<std::string> urls;
			std::map<std::string, std::vector<std::string>> tags;
			std::set<std::string> alltags;
			bool offline;
	};

	class file_urlreader : public urlreader {
		public:
			file_urlreader(const std::string& file = "");
			virtual ~file_urlreader();
			virtual void write_config();
			virtual void reload();
			void load_config(const std::string& file);
			virtual std::string get_source();
		private:
			std::string filename;
	};

	class opml_urlreader : public urlreader {
		public:
			opml_urlreader(configcontainer * c);
			virtual ~opml_urlreader();
			virtual void write_config();
			virtual void reload();
			virtual std::string get_source();
		protected:
			virtual void handle_node(xmlNode * node, const std::string& tag);
			virtual const char * get_auth();
			configcontainer * cfg;
		private:
			void rec_find_rss_outlines(xmlNode * node, std::string tag);
	};

}


#endif
