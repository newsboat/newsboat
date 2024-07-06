#ifndef NEWSBOAT_FEEDBINURLREADER_H_
#define NEWSBOAT_FEEDBINURLREADER_H_

#include "filepath.h"
#include "urlreader.h"

namespace newsboat {

class RemoteApi;

class FeedbinUrlReader : public UrlReader {
public:
	FeedbinUrlReader(const Filepath& url_file, RemoteApi* a);
	std::optional<utils::ReadTextFileError> reload() override;
	std::string get_source() override;

private:
	Filepath file;
	RemoteApi* api;
};

} // namespace newsboat

#endif /* NEWSBOAT_FEEDBINURLREADER_H_ */

