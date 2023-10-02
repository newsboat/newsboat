#include "filepath.h"

namespace newsboat {

Filepath::Filepath()
	: rs_object(filepath::bridged::create_empty())
{
}

Filepath Filepath::from_locale_string(const std::string& filepath)
{
	rust::Vec<std::uint8_t> data;
	data.reserve(filepath.length());
	for (const auto byte : filepath) {
		data.push_back(byte);
	}

	Filepath result;
	result.rs_object = filepath::bridged::create(std::move(data));
	return result;
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
	auto result = clone();
	result.push(component);
	return result;
}

Filepath Filepath::clone() const
{
	Filepath result;
	result.rs_object = filepath::bridged::clone(*rs_object);
	return result;
}

bool Filepath::is_absolute() const
{
	return filepath::bridged::is_absolute(*rs_object);
};

} // namespace newsboat
