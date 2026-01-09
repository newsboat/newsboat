#include "stflrichtext.h"
#include "libnewsboat-ffi/src/stflrichtext.rs.h"

namespace newsboat {

StflRichText::StflRichText(rust::Box<stflrichtext::bridged::StflRichText>&& rs_object)
	: rs_object(std::move(rs_object))
{
}

StflRichText::StflRichText(const StflRichText& other)
	: rs_object(stflrichtext::bridged::copy(*other.rs_object))
{
}

StflRichText& StflRichText::operator=(const StflRichText& other)
{
	this->rs_object = stflrichtext::bridged::copy(*other.rs_object);
	return *this;
}

StflRichText StflRichText::from_plaintext(const std::string& text)
{
	auto rs_object = stflrichtext::bridged::from_plaintext(text);

	return StflRichText(std::move(rs_object));
}

StflRichText StflRichText::from_plaintext_with_style(const std::string& text,
	const std::string& style_tag)
{
	auto rs_object = stflrichtext::bridged::from_plaintext_with_style(text, style_tag);

	return StflRichText(std::move(rs_object));
}

StflRichText StflRichText::from_quoted(const std::string& text)
{
	auto rs_object = stflrichtext::bridged::from_quoted(text);

	return StflRichText(std::move(rs_object));
}

void StflRichText::append(const StflRichText& other)
{
	stflrichtext::bridged::append(*rs_object, *other.rs_object);
}

void StflRichText::highlight_searchphrase(const std::string& search, bool case_insensitive)
{
	stflrichtext::bridged::highlight_searchphrase(*rs_object, search, case_insensitive);
}

void StflRichText::apply_style_tag(const std::string& tag, size_t start, size_t end)
{
	stflrichtext::bridged::apply_style_tag(*rs_object, tag, start, end);
}

std::string StflRichText::plaintext() const
{
	return std::string(stflrichtext::bridged::plaintext(*rs_object));
}

std::string StflRichText::stfl_quoted() const
{
	return std::string(stflrichtext::bridged::quoted(*rs_object));
}

}
