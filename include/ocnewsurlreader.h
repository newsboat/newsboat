#ifndef NEWSBOAT_OCNEWSURLREADER_H_
#define NEWSBOAT_OCNEWSURLREADER_H_

#include "urlreader.h"
#include "utf8string.h"

namespace newsboat {

class RemoteApi;

class OcNewsUrlReader : public UrlReader {
public:
	OcNewsUrlReader(const std::string& url_file, RemoteApi* a);
	~OcNewsUrlReader() override;
	nonstd::optional<std::string> reload() override;
	std::string get_source() override;

private:
	Utf8String file;
	RemoteApi* api;
};

} // namespace newsboat

#endif /* NEWSBOAT_OCNEWSURLREADER_H_ */
