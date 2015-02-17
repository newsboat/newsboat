#include <markreadthread.h>
#include <logger.h>

namespace newsbeuter
{

markreadthread::markreadthread( ttrss_api* r_api, const std::string& guid, bool read ) :
	_r_api( r_api ),
	_guid( guid ),
	_read( read ) {
}

markreadthread::~markreadthread() {
}

void markreadthread::operator()() {
	/*
	 * the markreadthread class handles marking a thread as read in a way that
	 * doesn't halt the presentation of the content. this is for remote sources,
	 * like TinyTinyRSS.
	 */

	LOG(LOG_DEBUG, "markreadthread::run: inside markreadthread, marking thread as read...");

	// Call the ttrss_api's update_article function as a thread.
	_r_api->update_article( _guid, 2, _read ? 0 : 1 );

}

}
