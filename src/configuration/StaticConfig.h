#pragma once

#include <QString>
#include <unordered_map>
#include <utility>

using AuthorAliasesMap = std::unordered_map<QString, QString>;
using ExcludedPathSections = std::vector<QString>;

struct StaticConfig
{
	StaticConfig() = default;
	StaticConfig(AuthorAliasesMap authorAliases, QString copyrightFieldTemplate,
	             ExcludedPathSections excludedPathSections)
	    : m_authorAliases(std::move(authorAliases))
	    , m_copyrightFieldTemplate(std::move(copyrightFieldTemplate))
	    , m_excludedPathSections(std::move(excludedPathSections))
	{}

	[[nodiscard]] const auto &authorAliases() const { return m_authorAliases; }
	[[nodiscard]] const auto &copyrightFieldTemplate() const { return m_copyrightFieldTemplate; }
	[[nodiscard]] const auto &excludedPathSections() const { return m_excludedPathSections; }

private:
	AuthorAliasesMap m_authorAliases;
	QString m_copyrightFieldTemplate;
	ExcludedPathSections m_excludedPathSections;
};