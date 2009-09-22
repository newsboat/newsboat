#ifndef NEWSBEUTER_REMOTE_API__H
#define NEWSBEUTER_REMOTE_API__H

#include <configcontainer.h>

namespace newsbeuter {

class remote_api {
	public:
		remote_api(configcontainer * c) : cfg(c) { }
		virtual ~remote_api() { }
		virtual bool authenticate() = 0;
		// TODO
	protected:
		configcontainer * cfg;
};

}

#endif
