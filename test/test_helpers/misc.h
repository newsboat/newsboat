#ifndef NEWSBOAT_TEST_HELPERS_MISC_H_
#define NEWSBOAT_TEST_HELPERS_MISC_H_

#include <cstdint>
#include <string>
#include <vector>

#include "filepath.h"

namespace test_helpers {

/*
 * The assert_article_file_content opens a file where the content of an article
 * was previously dumped (using for example OP_SHOWURL, or OP_OPEN with an
 * appropriate "pager" config value ) and checks its content according to the
 * expected values passed as parameters.
 */
void assert_article_file_content(const newsboat::Filepath& path,
	const std::string& title,
	const std::string& author,
	const std::string& date,
	const std::string& url,
	const std::string& description);

/* \brief Copy a file
 */
void copy_file(const newsboat::Filepath& source, const newsboat::Filepath& destination);

/* \brief Returns the contents of the file at `filepath` (each line represented
 * by a separate string, without the newline character), or an empty vector if
 * the file couldn't be opened.
 */
std::vector<std::string> file_contents(const newsboat::Filepath& filepath);

/* \brief Returns the contents of the file as a single binary blob
 */
std::vector<std::uint8_t> read_binary_file(const newsboat::Filepath& filepath);

/* \brief Returns `true` if `input` starts with `prefix`.
 */
bool starts_with(const std::string& prefix, const std::string& input);

/* \brief Returns `true` if `input` ends with `suffix`.
 */
bool ends_with(const std::string& suffix, const std::string& input);

/* \brief Returns `true` if the file at `filepath` exists.
 */
bool file_exists(const newsboat::Filepath& filepath);

/* \brief Returns 0 on success, or -1 if an error occured.
 */
int mkdir(const newsboat::Filepath& dirpath, mode_t mode);

/* \brief Return `true` if file available for reading.
 */
bool file_available_for_reading(const newsboat::Filepath& filepath);

} // namespace test_helpers

#endif /* NEWSBOAT_TEST_HELPERS_MISC_H_ */
