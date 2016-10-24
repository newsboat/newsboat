#ifndef NEWSBEUTER_HTMLRENDERER__H
#define NEWSBEUTER_HTMLRENDERER__H

#include <textformatter.h>
#include <vector>
#include <string>
#include <istream>
#include <map>

namespace newsbeuter {

enum link_type { LINK_HREF, LINK_IMG, LINK_EMBED };
enum htmltag {
	TAG_A = 1, TAG_EMBED, TAG_BR, TAG_PRE, TAG_ITUNESHACK,
	TAG_IMG, TAG_BLOCKQUOTE, TAG_H1, TAG_H2, TAG_H3, TAG_H4,
	TAG_P, TAG_OL, TAG_UL, TAG_LI, TAG_DT, TAG_DD, TAG_DL,
	TAG_SUP, TAG_SUB, TAG_HR, TAG_STRONG, TAG_UNDERLINE, TAG_QUOTATION,
	TAG_SCRIPT, TAG_STYLE, TAG_TABLE, TAG_TH, TAG_TR, TAG_TD
};

typedef std::pair<std::string,link_type> linkpair;

class htmlrenderer {
	public:
		htmlrenderer(bool raw = false);
		void render(const std::string& source,
				std::vector<std::pair<LineType, std::string>>& lines,
				std::vector<linkpair>& links,
				const std::string& url);
		void render(std::istream & input,
				std::vector<std::pair<LineType, std::string>>& lines,
				std::vector<linkpair>& links,
				const std::string& url);
		static std::string render_hr(const unsigned int width);
		// only public for unit testing purposes:
		std::string format_ol_count(unsigned int count, char type);

		struct TableCell {
			TableCell(size_t s) : span(s) { }
			size_t span;
			std::vector<std::string> text; // multiline cell text
		};

		struct TableRow {
			TableRow() : inside(false) { }

			void add_text(const std::string& str);
			void start_cell(size_t span);
			void complete_cell();

			bool inside;    // inside a cell
			std::vector<TableCell> cells;
		};

		struct Table {
			Table(bool b) : inside(false), border(b) { }

			void add_text(const std::string& str);
			void start_row();
			void complete_row();
			void start_cell(size_t span);
			void complete_cell();

			bool inside;    // inside a row
			bool border;
			std::vector<TableRow> rows;
		};

	private:
		void prepare_new_line(std::string& line, int indent_level);
		bool line_is_nonempty(const std::string& line);
		unsigned int add_link(std::vector<linkpair>& links, const std::string& link, link_type type);
		std::string quote_for_stfl(std::string str);
		std::string absolute_url(const std::string& url, const std::string& link);
		std::string type2str(link_type type);
		std::map<std::string, htmltag> tags;
		void render_table(
				const Table& table,
				std::vector<std::pair<LineType, std::string>>& lines);
		void add_nonempty_line(
				const std::string& curline,
				std::vector<Table>& tables,
				std::vector<std::pair<LineType, std::string>>& lines);
		void add_line(
				const std::string& curline,
				std::vector<Table>& tables,
				std::vector<std::pair<LineType, std::string>>& lines);
		void add_line_softwrappable(
				const std::string& line,
				std::vector<std::pair<LineType, std::string>>& lines);
		void add_line_nonwrappable(
				const std::string& line,
				std::vector<std::pair<LineType, std::string>>& lines);
		void add_hr(std::vector<std::pair<LineType, std::string>>& lines);
		std::string get_char_numbering(unsigned int count);
		std::string get_roman_numbering(unsigned int count);
		bool raw_;
};

}

#endif
