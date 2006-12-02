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
	if (attributes.size() > 0) {
		attributes.erase(attributes.begin(), attributes.end());	
	}
	text = "";
	
	switch (current_event) {
		case START_DOCUMENT:
			char c = skip_whitespace();
			if (EOF == c) {
				current_event = END_DOCUMENT;
				break;
			}
			/* fall-through is intended */
		case START_TAG: {
				std::string s;
				char c;
				*inputstream >> c;
				while (!inputstream->eof() && c != '>') {
					*inputstream >> c;
					if (!inputstream->eof()) {
						s.append(1,c);
					}
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
				}	
			}
			break;
		case END_TAG:
			break;
		case TEXT:
			break;
		case END_DOCUMENT:	
			break;
	}
	// TODO: implement parser
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
	if (inputstream->eof()) {
		c = EOF;	
	}
	return c;
}

std::vector<std::string> xmlpullparser::tokenize(const std::string& s) {
	std::vector<std::string> result;
	// TODO: tokenize s
	return result;	
}

void xmlpullparser::add_attribute(const std::string& s) {
	// TODO: parse and add attribute	
}

}
