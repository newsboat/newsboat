#ifndef NEWSBEUTER_REMOTE_API__H
#define NEWSBEUTER_REMOTE_API__H

#include <configcontainer.h>
#include <string>
#include <utility>
#include <vector>

namespace newsbeuter {

typedef std::pair<std::string, std::vector<std::string> > tagged_feedurl;

class remote_api {
	public:
		remote_api(configcontainer * c) : cfg(c) { }
		virtual ~remote_api() { }
		virtual bool authenticate() = 0;
		virtual std::vector<tagged_feedurl> get_subscribed_urls() = 0;
		// TODO
	protected:
		configcontainer * cfg;
};

}

#endif
