#ifndef NEWSBOAT_HTMLRENDERER_H_
#define NEWSBOAT_HTMLRENDERER_H_

#include <istream>
#include <map>
#include <string>
#include <vector>

#include "textformatter.h"

namespace newsboat {

enum class link_type { HREF, IMG, EMBED };
enum class htmltag {
	A = 1,
	EMBED,
	BR,
	PRE,
	ITUNESHACK,
	IMG,
	BLOCKQUOTE,
	H1,
	H2,
	H3,
	H4,
	H5,
	H6,
	P,
	OL,
	UL,
	LI,
	DT,
	DD,
	DL,
	SUP,
	SUB,
	HR,
	STRONG,
	UNDERLINE,
	QUOTATION,
	SCRIPT,
	STYLE,
	TABLE,
	TH,
	TR,
	TD
};

typedef std::pair<std::string, link_type> linkpair;

class htmlrenderer {
public:
	explicit htmlrenderer(bool raw = false);
	void
	render(const std::string& source,
	       std::vector<std::pair<LineType, std::string>>& lines,
	       std::vector<linkpair>& links,
	       const std::string& url);
	void
	render(std::istream& input,
	       std::vector<std::pair<LineType, std::string>>& lines,
	       std::vector<linkpair>& links,
	       const std::string& url);
	static std::string render_hr(const unsigned int width);
	// only public for unit testing purposes:
	std::string format_ol_count(unsigned int count, char type);

	struct TableCell {
		explicit TableCell(size_t s)
			: span(s)
		{
		}
		size_t span;
		std::vector<std::string> text; // multiline cell text
	};

	struct TableRow {
		TableRow()
			: inside(false)
		{
		}

		void add_text(const std::string& str);
		void start_cell(size_t span);
		void complete_cell();

		bool inside; // inside a cell
		std::vector<TableCell> cells;
	};

	struct Table {
		explicit Table(bool b)
			: inside(false)
			, has_border(b)
		{
		}

		void add_text(const std::string& str);
		void start_row();
		void complete_row();
		void start_cell(size_t span);
		void complete_cell();

		bool inside; // inside a row
		bool has_border;
		std::vector<TableRow> rows;
	};

private:
	void prepare_new_line(std::string& line, int indent_level);
	bool line_is_nonempty(const std::string& line);
	unsigned int add_link(
		std::vector<linkpair>& links,
		const std::string& link,
		link_type type);
	std::string quote_for_stfl(std::string str);
	std::string
	absolute_url(const std::string& url, const std::string& link);
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

} // namespace newsboat

#endif /* NEWSBOAT_HTMLRENDERER_H_ */
