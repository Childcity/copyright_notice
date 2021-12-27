#pragma once

#include <QString>
#include <unordered_map>

using AuthorAliasesMap = std::unordered_map<QString, QString>;

struct StaticConfig
{
	AuthorAliasesMap authorAliases;
};