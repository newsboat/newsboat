#ifndef NEWSBOAT_TEST_HELPERS_MISC_H_
#define NEWSBOAT_TEST_HELPERS_MISC_H_

#include <string>

namespace TestHelpers {

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

} // namespace TestHelpers

#endif /* NEWSBOAT_TEST_HELPERS_MISC_H_ */
