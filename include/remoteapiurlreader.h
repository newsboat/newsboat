#ifndef NEWSBOAT_REMOTEAPIURLREADER_H_
#define NEWSBOAT_REMOTEAPIURLREADER_H_

#include "urlreader.h"

namespace newsboat {

class RemoteApi;

class RemoteApiUrlReader : public UrlReader {
public:
	RemoteApiUrlReader(const std::string& source_name, const Filepath& url_file,
		RemoteApi& api);
	~RemoteApiUrlReader() override = default;
	std::optional<utils::ReadTextFileError> reload() override;
	std::string get_source() const override;

protected:
	// Can be overridden to specify remote-specific special feeds at the top of the feeds list
	virtual void add_special_feeds()
	{
	}

private:
	std::string source_name;
	Filepath file;
	RemoteApi& api;
};

} // namespace newsboat

#endif /* NEWSBOAT_REMOTEAPIURLREADER_H_ */

