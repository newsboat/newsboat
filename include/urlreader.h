#ifndef NEWSBEUTER_CONFIGREADER__H
#define NEWSBEUTER_CONFIGREADER__H

#include <vector>
#include <string>

namespace newsbeuter {

	class urlreader {
		public:
			urlreader(const std::string& file = "");
			~urlreader();
			void load_config(const std::string& file);
			void write_config();
			std::vector<std::string>& get_urls();
			void reload();
		private:
			std::vector<std::string> urls;
			std::string filename;
	};

}


#endif
