#ifndef NEWSBEUTER_CONFIGREADER__H
#define NEWSBEUTER_CONFIGREADER__H

#include <vector>
#include <map>
#include <set>
#include <string>

#include <configcontainer.h>
#include <nxml.h>

namespace newsbeuter {

	class urlreader {
		public:
			urlreader();
			virtual ~urlreader();
			virtual void write_config() = 0;
			virtual void reload() = 0;
			virtual const std::string& get_source() = 0;
			std::vector<std::string>& get_urls();
			std::vector<std::string>& get_tags(const std::string& url);
			std::vector<std::string> get_alltags();
		protected:
			std::vector<std::string> urls;
			std::map<std::string, std::vector<std::string> > tags;
			std::set<std::string> alltags;
	};

	class file_urlreader : public urlreader {
		public:
			file_urlreader(const std::string& file = "");
			virtual ~file_urlreader();
			virtual void write_config();
			virtual void reload();
			void load_config(const std::string& file);
			virtual const std::string& get_source();
		private:
			std::string filename;
	};

	class bloglines_urlreader : public urlreader {
		public:
			bloglines_urlreader(configcontainer * c);
			virtual ~bloglines_urlreader();
			virtual void write_config();
			virtual void reload();
			virtual const std::string& get_source();
		private:
			void rec_find_rss_outlines(nxml_data_t * node, std::string tag);
			configcontainer * cfg;
			std::string listsubs_url;
			std::string getitems_url;
	};

}


#endif
