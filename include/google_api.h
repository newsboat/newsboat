#ifndef NEWSBEUTER_GOOGLE_API__H
#define NEWSBEUTER_GOOGLE_API__H

#include <remote_api.h>
#include <urlreader.h>

namespace newsbeuter {

class googlereader_api : public remote_api {
	public:
		googlereader_api(configcontainer * c);
		virtual ~googlereader_api();
		virtual bool authenticate();
};

class googlereader_urlreader : public urlreader {
	public:
		googlereader_urlreader(configcontainer * c, remote_api * a);
		virtual ~googlereader_urlreader();
		virtual void write_config();
		virtual void reload();
		virtual std::string get_source();
	private:
		configcontainer * cfg;
		remote_api * api;

};

}

#endif
