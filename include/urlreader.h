#ifndef NEWSBEUTER_CONFIGREADER__H
#define NEWSBEUTER_CONFIGREADER__H

#include <vector>
#include <map>
#include <set>
#include <string>

namespace newsbeuter {

	class urlreader {
		public:
			urlreader(const std::string& file = "");
			~urlreader();
			void load_config(const std::string& file);
			void write_config();
			std::vector<std::string>& get_urls();
			std::vector<std::string>& get_tags(const std::string& url);
			std::vector<std::string> get_alltags();
			void reload();
		private:
			std::vector<std::string> urls;
			std::map<std::string, std::vector<std::string> > tags;
			std::set<std::string> alltags;
			std::string filename;
	};

}


#endif
