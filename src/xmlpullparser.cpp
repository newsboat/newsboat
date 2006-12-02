#include <xmlpullparser.h>
#include <stdexcept>
#include <istream>

namespace noos
{

xmlpullparser::xmlpullparser() : inputstream(0), current_event(START_DOCUMENT)
{
}

xmlpullparser::~xmlpullparser()
{
}

void xmlpullparser::setInput(std::istream& is) {
	inputstream = &is;	
	current_event = START_DOCUMENT;
}

int xmlpullparser::getAttributeCount() const {
	if (START_TAG != current_event)
		return -1;
	return attributes.size();
}

std::string xmlpullparser::getAttributeName(unsigned int index) const {
	if (index >= attributes.size())
		throw std::out_of_range("invalid attribute index");
	return attributes[index].first;
}

std::string xmlpullparser::getAttributeValue(unsigned int index) const {
	if (index >= attributes.size())
		throw std::out_of_range("invalid attribute index");
	return attributes[index].second;	
}

std::string xmlpullparser::getAttributeValue(const std::string& name) const {
	for (std::vector<attribute>::const_iterator it=attributes.begin();it!=attributes.end();++it) {
		if (it->first == name) {
			return it->second;
		}	
	}	
	throw std::invalid_argument("attribute not found");
}

xmlpullparser::event xmlpullparser::getEventType() const {
	return current_event;	
}

std::string xmlpullparser::getText() const {
	return text;	
}

bool xmlpullparser::isWhitespace() const {
	bool found_nonws = false;
	for (unsigned int i=0;i<text.length();++i) {
		if (!isspace(text[i]))
			found_nonws = true;
	}
	return !found_nonws;
}

xmlpullparser::event xmlpullparser::next() {
	// TODO: refactor this
	if (attributes.size() > 0) {
		attributes.erase(attributes.begin(), attributes.end());	
	}
	text = "";
	
	switch (current_event) {
		case START_DOCUMENT: 
			{
				char c = skip_whitespace();
				if (inputstream->eof()) {
					current_event = END_DOCUMENT;
					break;
				}
				if (c != '<') {
					// TODO: throw exception that '<' was expected	
				}
				std::string s = read_tag();
				
				if (s.find("?xml",0) == 0) {
					c = skip_whitespace();
					if (inputstream->eof()) {
						current_event = END_DOCUMENT;
						break;
					}
					s = read_tag();
				}
				
				std::vector<std::string> tokens = tokenize(s);
				if (tokens.size() > 0) {
					text = tokens[0];
					std::vector<std::string>::iterator it = tokens.begin();
					++it;
					while (it != tokens.end()) {
						add_attribute(*it);
						++it;	
					}
				} else {
					// TODO: throw exception that an empty tag was found	
				}
				current_event = determine_tag_type();
			}	
			break;
		case START_TAG:
		case END_TAG:
			{
				char c;
				*inputstream >> c;
				if (c == '<') {
					std::string s = read_tag();
					std::vector<std::string> tokens = tokenize(s);
					if (tokens.size() > 0) {
						text = tokens[0];
						std::vector<std::string>::iterator it = tokens.begin();
						++it;
						while (it != tokens.end()) {
							add_attribute(*it);
							++it;	
						}
					} else {
						// TODO: throw exception that an empty tag was found	
					}
					current_event = determine_tag_type();
				} else {
					// read text
					while (!inputstream->eof() && c != '<') {
						text.append(1,c);
					}
					if (inputstream->eof()) {
						// TODO: throw exception that EOF was hit before any end tag
					}
					current_event = TEXT;
				}
			}
			break;
		case TEXT:
			{
				std::string s = read_tag();
				std::vector<std::string> tokens = tokenize(s);
				if (tokens.size() > 0) {
					text = tokens[0];
					std::vector<std::string>::iterator it = tokens.begin();
					++it;
					while (it != tokens.end()) {
						add_attribute(*it);
						++it;	
					}
				} else {
					// TODO: throw exception that an empty tag was found	
				}
				current_event = determine_tag_type();	
			}
			break;
		case END_DOCUMENT:	
			// nothing
			break;
	}
	return getEventType();	
}

int xmlpullparser::skip_whitespace() {
	int c = EOF;
	while (!inputstream->eof()) {
		char x;
		*inputstream >> x;
		c = x;
		if (!isspace(c))
			break;
	}
	return c;
}

std::vector<std::string> xmlpullparser::tokenize(const std::string& str, const std::string& delimiters) {
    std::vector<std::string> tokens;
    std::string::size_type last_pos = str.find_first_not_of(delimiters, 0);
    std::string::size_type pos = str.find_first_of(delimiters, last_pos);

    while (std::string::npos != pos || std::string::npos != last_pos) {
            tokens.push_back(str.substr(last_pos, pos - last_pos));
            last_pos = str.find_first_not_of(delimiters, pos);
            pos = str.find_first_of(delimiters, last_pos);
    }
    return tokens;
}

void xmlpullparser::add_attribute(const std::string& s) {
	std::vector<std::string> tokens = tokenize(s,"=");
	std::string attribname, attribvalue;
	if (tokens.size() > 0) {
		attribname = tokens[0];
		if (tokens.size() > 1)
			attribvalue = tokens[1];
		else
			attribvalue = tokens[0];
	}
	// TODO: do proper attribute decoding
	attributes.push_back(attribute(attribname,attribvalue));
}

std::string xmlpullparser::read_tag() {
	char c;
	std::string s;
	*inputstream >> c;
	while (!inputstream->eof() && c != '>') {
		*inputstream >> c;
		if (!inputstream->eof()) {
			s.append(1,c);
		}
		*inputstream >> c;
	}
	if (inputstream->eof()) {
		// TODO: throw exception that EOF was hit before '>'	
	}
	return s;
}

xmlpullparser::event xmlpullparser::determine_tag_type() {
	if (text.length() > 0 && text[0] == '/') {
		text.erase(0,1);
		return END_TAG;
	}
	return START_TAG;	
}

}
