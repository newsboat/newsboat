#ifndef NOOS_CONFIGREADER__H
#define NOOS_CONFIGREADER__H

#include <vector>
#include <string>

namespace noos {

	class configreader {
		public:
			configreader(const std::string& file = "");
			~configreader();
			void load_config(const std::string& file);
			const std::vector<std::string>& get_urls();
			void reload();
		private:
			std::vector<std::string> urls;
			std::string filename;
	};

}


#endif
