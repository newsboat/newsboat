#include "filestring.h"
/*
 * http://www.zedwood.com/article/cpp-is-valid-utf8-string-function
 */
bool FileString::is_valid_utf8(const std::vector<std::uint8_t>& str) const
{
	int c, i, ix, n, j;
	for (i = 0, ix = str.size(); i < ix; i++) {
		c = (unsigned char) str[i];
		//if (c==0x09 || c==0x0a || c==0x0d || (0x20 <= c && c <= 0x7e) ) n = 0; // is_printable_ascii
		if (0x00 <= c && c <= 0x7f) {
			n=0;        // 0bbbbbbb
		} else if ((c & 0xE0) == 0xC0) {
			n=1;        // 110bbbbb
		} else if ( c==0xed && i<(ix-1)
			&& ((unsigned char)str[i+1] & 0xa0)==0xa0) { // Probably a surrogate check
			return false;        //U+d800 to U+dfff
		} else if ((c & 0xF0) == 0xE0) {
			n=2;        // 1110bbbb
		} else if ((c & 0xF8) == 0xF0) {
			n=3;        // 11110bbb
		}
		//else if (($c & 0xFC) == 0xF8) n=4; // 111110bb //byte 5, unnecessary in 4 byte UTF-8
		//else if (($c & 0xFE) == 0xFC) n=5; // 1111110b //byte 6, unnecessary in 4 byte UTF-8
		else {
			return false;
		}
		for (j=0; j<n && i<ix; j++) { // n bytes matching 10bbbbbb follow ?
			if ((++i == ix) || (( (unsigned char)str[i] & 0xC0) != 0x80)) {
				return false;
			}
		}
	}
	return true;
}

void FileString::to_valid_utf8(std::vector<std::uint8_t>& str) const
{
	int c, i, ix, n = 0, j;
	for (i = 0, ix = str.size(); i < ix; i++) {
		c = (unsigned char) str[i];
		//if (c==0x09 || c==0x0a || c==0x0d || (0x20 <= c && c <= 0x7e) ) n = 0; // is_printable_ascii
		if (0x00 <= c && c <= 0x7f) {
			n=0;        // 0bbbbbbb
		} else if ((c & 0xE0) == 0xC0) {
			n=1;        // 110bbbbb
		} else if ( c==0xed && i<(ix-1)
			&& ((unsigned char)str[i+1] & 0xa0)==0xa0) { // Probably a surrogate check
			str[i] = 0x80;        //U+d800 to U+dfff
		} else if ((c & 0xF0) == 0xE0) {
			n=2;        // 1110bbbb
		} else if ((c & 0xF8) == 0xF0) {
			n=3;        // 11110bbb
		}
		//else if (($c & 0xFC) == 0xF8) n=4; // 111110bb //byte 5, unnecessary in 4 byte UTF-8
		//else if (($c & 0xFE) == 0xFC) n=5; // 1111110b //byte 6, unnecessary in 4 byte UTF-8
		else {
			str[i] = 0x80;
		}
		for (j=0; j<n && i<ix; j++) { // n bytes matching 10bbbbbb follow ?
			if ((++i == ix) || (( (unsigned char)str[i] & 0xC0) != 0x80)) {
				str[i] = 0x80;
			}
		}
	}
}
