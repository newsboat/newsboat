#include <xmlpullparser.h>
#include <exceptions.h>
#include <stdexcept>
#include <istream>
#include <sstream>
#include <iostream>

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
	
	if (inputstream->eof()) {
		current_event = END_DOCUMENT;
		return current_event;	
	}
	
	switch (current_event) {
		case START_DOCUMENT: 
			{
				char c = skip_whitespace();
				if (inputstream->eof()) {
					current_event = END_DOCUMENT;
					break;
				}
				if (c != '<') {
					// read text
					while (!inputstream->eof() && c != '<') {
						text.append(1,c);
						// *inputstream >> c;
						inputstream->read(&c,1);
					}
					remove_trailing_whitespace(text);
					current_event = TEXT;
				} else {
					std::string s;
					try {
						s = read_tag();
					} catch (const xmlexception &) {
						current_event = END_DOCUMENT;
						break;
					}
					
					if (s.find("?xml",0) == 0) {
						c = skip_whitespace();
						if (inputstream->eof()) {
							current_event = END_DOCUMENT;
							break;
						}
						try {
							s = read_tag();
						} catch (const xmlexception &) {
							current_event = END_DOCUMENT;
							break;
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
					} else {
						throw xmlexception("empty tag found");	
					}
					current_event = determine_tag_type();
				}
			}	
			break;
		case START_TAG:
		case END_TAG:
			{
				char c;
				*inputstream >> c;
				if (!inputstream->eof()) {
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
							throw xmlexception("empty tag found");	
						}
						current_event = determine_tag_type();
					} else {
						// read text
						while (!inputstream->eof() && c != '<') {
							text.append(1,c);
							// *inputstream >> c;
							inputstream->read(&c,1);
						}
						remove_trailing_whitespace(text);
						current_event = TEXT;
					}
				} else {
					current_event = END_DOCUMENT;
				}
			}
			break;
		case TEXT:
			{
				std::string s;
				try {
					s = read_tag();
				} catch (const xmlexception &) {
					current_event = END_DOCUMENT;
					break;
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
					throw xmlexception("empty tag found");	
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
	char c;
	while (!inputstream->eof()) {
		inputstream->read(&c,1);
		if (!isspace(c))
			break;
	}
	return c;
}

std::vector<std::string> xmlpullparser::tokenize(const std::string& str, std::string delimiters) {
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
	std::string::size_type equalpos = s.find_first_of("=",0);
	std::string attribname, attribvalue;
	
	if (equalpos != std::string::npos) {
		attribname = s.substr(0,equalpos);
		attribvalue = s.substr(equalpos+1,s.length()-(equalpos+1));
	} else {
		attribname = attribvalue = s;
	}
	attribvalue = decode_attribute(attribvalue);
	// std::cerr << "add_attribute: " << attribname << "=" << attribvalue << std::endl;
	attributes.push_back(attribute(attribname,attribvalue));
}

std::string xmlpullparser::read_tag() {
	char c;
	std::string s;
	inputstream->read(&c,1);
	while (!inputstream->eof() && c != '>') {
		s.append(1,c);
		// std::cout << "read `" << c << "'" << std::endl;
		inputstream->read(&c,1);
	}
	if (inputstream->eof()) {
		throw xmlexception("EOF found while reading XML tag");	
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

std::string xmlpullparser::decode_attribute(const std::string& s) {
	std::string s1 = s;
	if ((s1[0] == '"' && s1[s1.length()-1] == '"') || (s1[0] == '\'' && s1[s1.length()-1] == '\'')) {
		if (s1.length() > 0)
			s1.erase(0,1);
		if (s1.length() > 0)
			s1.erase(s1.length()-1,1);	
	}
	return decode_entities(s1);
}

std::string xmlpullparser::decode_entities(const std::string& s) {
	std::string result, current_entity;
	bool reading_entity = false;
	unsigned int i = 0;
	while (i < s.length()) {
		if (reading_entity) {	
			if (s[i] == ';') {
				reading_entity = false;
				result.append(decode_entity(current_entity));
			} else {
				current_entity.append(1,s[i]);	
			}
		} else {
			if (s[i] == '&') {
				reading_entity = true;
				current_entity = "";
			} else {
				result.append(1,s[i]);	
			}
		}
		++i;
	}
	return result;
}

std::string xmlpullparser::decode_entity(std::string s) {
	// TODO: improve entity decoder
	if (s == "lt") {
		return "<";
	} else if (s == "gt") {
		return ">";
	} else if (s == "quot") {
		return "\"";
	} else if (s == "apos") {
		return "'";
	} else if (s == "amp") {
		return "&";
	} else if (s.length() > 1 && s[0] == '#') {
		std::string result;
		if (s[1] == 'x') {
			s.erase(0,2);
			std::istringstream is(s);
			unsigned int i;
			is >> std::hex >> i;
			result.append(1,static_cast<char>(i));
		} else {
			s.erase(0,1);
			std::istringstream is(s);
			unsigned int i;
			is >> i;
			result.append(1,static_cast<char>(i));
		}
		return result;
	}
	return ""; 	
}

void xmlpullparser::remove_trailing_whitespace(std::string& s) {
	while (s.length() > 0 && isspace(s[s.length()-1])) {
		s.erase(s.length()-1,1);	
	}
}

}
