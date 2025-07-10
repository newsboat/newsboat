#ifndef NEWSBOAT_REMOTEAPIURLREADER_H_
#define NEWSBOAT_REMOTEAPIURLREADER_H_

#include "urlreader.h"

namespace Newsboat {

class RemoteApi;

class RemoteApiUrlReader : public UrlReader {
public:
	RemoteApiUrlReader(const std::string& source_name, const std::string& url_file,
		RemoteApi& api);
	std::optional<utils::ReadTextFileError> reload() override;
	std::string get_source() const override;

private:
	std::string source_name;
	std::string file;
	RemoteApi& api;
};

} // namespace Newsboat

#endif /* NEWSBOAT_REMOTEAPIURLREADER_H_ */

