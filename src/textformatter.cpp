#include <textformatter.h>
#include <utils.h>
#include <strprintf.h>
#include <htmlrenderer.h>
#include <stflpp.h>
#include <assert.h>
#include <limits.h>
#include <algorithm>

namespace newsboat {

textformatter::textformatter() { }

textformatter::~textformatter() { }

void textformatter::add_line(LineType type, std::string line) {
	LOG(level::DEBUG,
		"textformatter::add_line: `%s' (line type %i)",
		line,
		type);

	auto clean_line =
		utils::wstr2str(
			utils::clean_nonprintable_characters(utils::str2wstr(line)));
	lines.push_back(std::make_pair(type, clean_line));
}

void textformatter::add_lines(
		const std::vector<std::pair<LineType, std::string>>& lines)
{
	for (auto line : lines) {
		add_line(line.first,
			utils::replace_all(line.second, "\t", "        "));
	}
}

std::vector<std::string> wrap_line(
		const std::string& line,
		const size_t width)
{
	if (line.empty()) {
		return { "" };
	}

	std::vector<std::string> result;
	std::vector<std::string> words = utils::tokenize_spaced(line);

	std::string prefix;
	size_t prefix_width = 0;
	auto iswhitespace =
		[](const std::string& input)
		{
			return std::all_of(
					input.cbegin(),
					input.cend(),
					[](std::string::value_type c) { return std::isspace(c); });
		};
	if (iswhitespace(words[0])) {
		prefix = utils::substr_with_width(words[0], width);
		prefix_width = utils::strwidth_stfl(prefix);
		words.erase(words.cbegin());
	}

	std::string curline = prefix;

	for (auto word : words) {
		size_t word_width = utils::strwidth_stfl(word);
		size_t curline_width = utils::strwidth_stfl(curline);

		// for languages (e.g., CJK) don't use a space as a word boundary
		while (word_width > (width - prefix_width)) {
			size_t space_left = width - curline_width;
			std::string part = utils::substr_with_width(word, space_left);
			curline.append(part);
			word.erase(0, part.length());
			result.push_back(curline);
			curline = prefix;
			if (part.empty()) {
				// discard the current word
				word.clear();
				word_width = 0;
			}

			word_width = utils::strwidth_stfl(word);
			curline_width = utils::strwidth_stfl(curline);
		}

		if ((curline_width + word_width) > width) {
			result.push_back(curline);
			if (iswhitespace(word)) {
				curline = prefix;
			} else {
				curline = prefix + word;
			}
		} else {
			curline.append(word);
		}
	}

	if (curline != prefix) {
		result.push_back(curline);
	}

	return result;
}

std::vector<std::string> format_text_plain_helper(
		const std::vector<std::pair<LineType, std::string>>& lines,
		regexmanager * rxman,
		const std::string& location,
		// wrappable lines are wrapped at this width
		const size_t wrap_width,
		// if non-zero, softwrappable lines are wrapped at this width
		const size_t total_width)
{
	LOG(level::DEBUG,
		"textformatter::format_text_plain: rxman = %p, location = `%s', "
		"wrap_width = %zu, total_width = %zu, %u lines",
		rxman, location, wrap_width, total_width, lines.size());

	std::vector<std::string> format_cache;

	auto store_line =
		[&format_cache]
		(std::string line) {
			format_cache.push_back(line);

			LOG(level::DEBUG,
				"textformatter::format_text_plain: stored `%s'",
				line);
		};

	for (auto line : lines) {
		auto type = line.first;
		auto text = line.second;

		LOG(level::DEBUG,
			"textformatter::format_text_plain: got line `%s' type %u",
			text, type);

		if (rxman && type != LineType::hr) {
			rxman->quote_and_highlight(text, location);
		}

		switch(type) {
			case LineType::wrappable:
				if(text == "") {
					store_line(" ");
					continue;
				}
				text = utils::consolidate_whitespace(text);
				for (auto line : wrap_line(text, wrap_width)) {
					store_line(line);
				}
				break;

			case LineType::softwrappable:
				if(text == "") {
					store_line(" ");
					continue;
				}
				if (total_width == 0) {
					store_line(text);
				} else {
					for (auto line : wrap_line(text, total_width)) {
						store_line(line);
					}
				}
				break;

			case LineType::nonwrappable:
				store_line(text);
				break;

			case LineType::hr:
				store_line(htmlrenderer::render_hr(wrap_width));
				break;
		}
	}

	return format_cache;
}

std::pair<std::string, std::size_t>
textformatter::format_text_to_list(
		regexmanager * rxman,
		const std::string& location,
		const size_t wrap_width,
		const size_t total_width)
{
	auto formatted = format_text_plain_helper(
			lines, rxman, location, wrap_width, total_width);

	auto format_cache = std::string("{list");
	for (auto line : formatted) {
		if (line != "") {
			utils::trim_end(line);
			format_cache.append(
					strprintf::fmt(
						"{listitem text:%s}",
						stfl::quote(line)));
		}
	}
	format_cache.append(1, '}');

	auto line_count = formatted.size();

	return { format_cache, line_count };
}

std::string textformatter::format_text_plain(
		const size_t width, const size_t total_width)
{
	std::string result;
	auto formatted = format_text_plain_helper(
			lines, nullptr, "", width, total_width);
	for (const auto& line : formatted) {
		result += line + "\n";
	}

	return result;
}

}
