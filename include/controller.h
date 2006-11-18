#ifndef NOOS_CONTROLLER__H
#define NOOS_CONTROLLER__H

#include <configreader.h>
#include <rss.h>
#include <cache.h>

namespace noos {

	class view;

	class controller {
		public:
			controller();
			~controller();
			void set_view(view * vv);
			void run(int argc = 0, char * argv[] = NULL);
			void open_feed(unsigned int pos);
			void open_item(rss_item& item);
			void reload(unsigned int pos);
			void reload_all();
		private:
			void usage(char * argv0);
			void import_opml(char * filename);
			void export_opml();

			view * v;
			configreader cfg;
			cache * rsscache;
			std::vector<rss_feed> feeds;
	};

}


#endif
