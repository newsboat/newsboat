#ifndef FILE_STRING_H_
#define FILE_STRING_H_
#include <algorithm>
#include <vector>
#include <string>
class FileString {
private:
	std::vector<std::uint8_t> str;
	bool is_utf8;

	bool is_valid_utf8(const std::vector<std::uint8_t>& str) const;

	void to_valid_utf8(std::vector<std::uint8_t>& str) const;

public:
	// Constructors
	// Store as raw bytes and validate utf8
	FileString(const std::vector<std::uint8_t>& path)
	{
		str = path;
		is_utf8 = is_valid_utf8(str);
	}

	FileString(const std::string& path)
	{
		str.reserve(path.size());
		std::copy(path.begin(), path.end(), std::back_inserter(str));
		is_utf8 = is_valid_utf8(str);
	}

	// Set invalid bytes to \x80 if is_utf8 false
	// otherwise just create a string from vector and return
	std::string to_utf8() const
	{
		auto tmp = str;
		to_valid_utf8(tmp);
		return std::string(tmp.begin(), tmp.end());
	}

	operator std::string() const
	{
		return std::string(str.begin(), str.end());
	}
};
#endif
