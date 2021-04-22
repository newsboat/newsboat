#ifndef NEWSBOAT_TEST_HELPERS_MISC_H_
#define NEWSBOAT_TEST_HELPERS_MISC_H_

#include <string>
#include <vector>

namespace test_helpers {

/*
 * The assert_article_file_content opens a file where the content of an article
 * was previously dumped (using for example OP_SHOWURL, or OP_OPEN with an
 * appropriate "pager" config value ) and checks its content according to the
 * expected values passed as parameters.
 */
void assert_article_file_content(const std::string& path,
	const std::string& title,
	const std::string& author,
	const std::string& date,
	const std::string& url,
	const std::string& description);

/* \brief Copy a file
 */
void copy_file(const std::string& source, const std::string& destination);

/* \brief Returns the contents of the file at `filepath` (each line represented
 * by a separate string, without the newline character), or an empty vector if
 * the file couldn't be opened.
 */
std::vector<std::string> file_contents(const std::string& filepath);

/* \brief Returns `true` if the file at `filepath` exists.
 */
bool file_exists(const std::string& filepath);

} // namespace test_helpers

#endif /* NEWSBOAT_TEST_HELPERS_MISC_H_ */
