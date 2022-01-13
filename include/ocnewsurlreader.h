#ifndef NEWSBOAT_OCNEWSURLREADER_H_
#define NEWSBOAT_OCNEWSURLREADER_H_

#include "urlreader.h"

namespace newsboat {

class RemoteApi;

class OcNewsUrlReader : public UrlReader {
public:
	OcNewsUrlReader(const std::string& url_file, RemoteApi* a);
	~OcNewsUrlReader() override;
	nonstd::optional<utils::ReadTextFileError> reload() override;
	std::string get_source() override;

private:
	std::string file;
	RemoteApi* api;
};

} // namespace newsboat

#endif /* NEWSBOAT_OCNEWSURLREADER_H_ */
