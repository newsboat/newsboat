#include "opts.h"

TestHelpers::Opts::Opts(std::initializer_list<std::string> opts)
	: m_argc(opts.size())
{
	m_opts.reserve(m_argc);

	for (const std::string& option : opts) {
		// Copy string into separate char[], managed by
		// unique_ptr.
		auto ptr = std::unique_ptr<char[]>(
				new char[option.size() + 1]);
		std::copy(option.cbegin(), option.cend(), ptr.get());
		// C and C++ require argv to be NULL-terminated:
		// https://stackoverflow.com/questions/18547114/why-do-we-need-argc-while-there-is-always-a-null-at-the-end-of-argv
		ptr.get()[option.size()] = '\0';

		// Hold onto the smart pointer to keep the entry in argv
		// alive.
		m_opts.emplace_back(std::move(ptr));
	}

	// Copy out intermediate argv vector into its final storage.
	m_data = std::unique_ptr<char* []>(new char* [m_argc + 1]);
	int i = 0;
	for (const auto& ptr : m_opts) {
		m_data.get()[i++] = ptr.get();
	}
	m_data.get()[i] = nullptr;
}

std::size_t TestHelpers::Opts::argc() const
{
	return m_argc;
}

char** TestHelpers::Opts::argv() const
{
	return m_data.get();
}
