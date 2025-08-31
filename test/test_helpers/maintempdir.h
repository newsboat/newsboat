#ifndef NEWSBOAT_TEST_HELPERS_MAINTEMPDIR_H_
#define NEWSBOAT_TEST_HELPERS_MAINTEMPDIR_H_

#include <exception>
#include <string>

#include "filepath.h"
// Users of this class might use the path in assertions, so let's include the
// string maker for their convenience.
#include "stringmaker/filepath.h"

namespace test_helpers {

/* Objects of MainTempDir class create Newsboat's temporary directory, and try
 * to remove it when they are destroyed. Other classes (TempFile and TempDir)
 * use this one to put their stuff in Newsboat's temp dir. */
class MainTempDir {
public:
	class tempfileexception : public std::exception {
	public:
		explicit tempfileexception(
			const newsboat::Filepath& filepath,
			const std::string& error);
		virtual const char* what() const throw();

	private:
		const std::string msg;
	};

	MainTempDir();

	~MainTempDir();

	newsboat::Filepath get_path() const;

private:
	newsboat::Filepath tempdir;
};

} // namespace test_helpers

#endif /* NEWSBOAT_TEST_HELPERS_MAINTEMPDIR_H_ */
