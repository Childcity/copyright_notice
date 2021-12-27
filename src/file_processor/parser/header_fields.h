#pragma once

#include <QString>

#include "src/constants.h"

namespace header_fields {

namespace impl {

static const QLatin1String file("File");
static const QLatin1String author("Author");
static const QLatin1String copyright("Copyright");
static const QLatin1String component("This file is part of");

inline int currentYear()
{
	const auto now = std::chrono::system_clock::now();
	const auto currentTime = std::chrono::system_clock::to_time_t(now);
	std::tm currentLocalTime{};
	localtime_s(&currentLocalTime, &currentTime);
	return currentLocalTime.tm_year + 1900;
}

}  // namespace impl

static const QLatin1String allWithSeparator("File|Author|Copyright|This file is part of");

// FieldType Also is order, using which fields will be serialized
enum FieldType { File, Author, Copyright, Component, Default = 100 };

// clang-format off
inline FieldType toFieldType(const QString &fieldName)
{
	if (fieldName == impl::file) { return FieldType::File; }
	if (fieldName == impl::author) { return FieldType::Author; }
	if (fieldName == impl::copyright) { return FieldType::Copyright; }
	if (fieldName == impl::component) { return FieldType::Component; }
	return Default;
}

inline QString toFieldName(FieldType fieldType)
{
	switch (fieldType) {
	case FieldType::File: return impl::file;
	case FieldType::Author: return impl::author;
	case FieldType::Copyright: return impl::copyright;
	case FieldType::Component: return impl::component;
	default: break;
	}
	assert(false);
	return {};
}
// clang-format on

inline QString makeCopyrightValue(QString copyrightTemplate)
{
	const auto year = impl::currentYear();
	return copyrightTemplate.replace(QLatin1String("%CURRENT_YEAR%"), QString::number(year));
}

constexpr bool isListField(FieldType field)
{
	return field == FieldType::Author;
}

constexpr bool requiresFullstop(FieldType field)
{
	return field == FieldType::Component;
}

}  // namespace header_fields