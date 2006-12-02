#ifndef XMLPULLPARSER_H_
#define XMLPULLPARSER_H_

#include <string>
#include <utility>
#include <vector>

namespace noos
{

class xmlpullparser
{
public:
	enum event { START_DOCUMENT, END_DOCUMENT, START_TAG, END_TAG, TEXT };
	
	xmlpullparser();
	virtual ~xmlpullparser();
	void setInput(std::istream& is);
	int getAttributeCount() const;
	std::string getAttributeName(unsigned int index) const;
	std::string getAttributeValue(unsigned int index) const;
	std::string getAttributeValue(const std::string& name) const;
	event getEventType() const;
	std::string getText() const;
	bool isWhitespace() const;
	event next();
private:
	typedef std::pair<std::string,std::string> attribute;
	std::vector<attribute> attributes;
	std::string text;
	std::istream * inputstream;
	event current_event;
	
	int skip_whitespace();
	std::vector<std::string> tokenize(const std::string& s);
	void add_attribute(const std::string& s);
	
};

}

#endif /*XMLPULLPARSER_H_*/
