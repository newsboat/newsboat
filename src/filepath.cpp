#define ENABLE_IMPLICIT_FILEPATH_CONVERSIONS

#include "filepath.h"

namespace newsboat {

inline namespace {

rust::Vec<std::uint8_t> string_to_vec(const std::string& input)
{
	rust::Vec<std::uint8_t> result;
	result.reserve(input.length());
	for (const auto byte : input) {
		result.push_back(byte);
	}
	return result;
}

}

Filepath::Filepath()
	: rs_object(filepath::bridged::create_empty())
{
}

Filepath Filepath::from_locale_string(const std::string& filepath)
{
	Filepath result;
	result.rs_object = filepath::bridged::create(string_to_vec(filepath));
	return result;
}

Filepath::Filepath(const Filepath& filepath)
	: rs_object(filepath::bridged::clone(*filepath.rs_object))
{
}

Filepath& Filepath::operator=(const Filepath& filepath)
{
	if (this == &filepath) {
		return *this;
	}
	rs_object = filepath::bridged::clone(*filepath.rs_object);
	return *this;
}

std::string Filepath::to_locale_string() const
{
	const auto bytes = filepath::bridged::into_bytes(*rs_object);
	return std::string(std::begin(bytes), std::end(bytes));
}

std::string Filepath::display() const
{
	return std::string(filepath::bridged::display(*rs_object));
}

bool Filepath::operator==(const Filepath& other) const
{
	return filepath::bridged::equals(*rs_object, *other.rs_object);
}

bool Filepath::operator!=(const Filepath& other) const
{
	return !(*this == other);
}

void Filepath::push(const Filepath& component)
{
	filepath::bridged::push(*rs_object, *component.rs_object);
}

Filepath Filepath::join(const Filepath& component) const
{
	auto result = *this;
	result.push(component);
	return result;
}

bool Filepath::is_absolute() const
{
	return filepath::bridged::is_absolute(*rs_object);
}

bool Filepath::set_extension(const std::string& ext)
{
	return filepath::bridged::set_extension(*rs_object, string_to_vec(ext));
}

bool Filepath::starts_with(const std::string& base) const
{
	return filepath::bridged::starts_with(*rs_object, string_to_vec(base));
}

std::optional<Filepath> Filepath::file_name() const
{
	auto str = filepath::bridged::file_name(*rs_object);
	auto res = std::string(str.begin(), str.end());
	if (res.empty()) {
		return std::nullopt;
	} else {
		return res;
	}
}

} // namespace newsboat
