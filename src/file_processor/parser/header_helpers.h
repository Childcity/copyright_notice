#pragma once

#include <optional>
#include <set>
#include <unordered_map>

#include "src/configuration/RunConfig.h"
#include "src/configuration/StaticConfig.h"
#include "src/file_processor/git/GitRepository.h"

namespace header_helpers {

using StringViewIterator = std::string_view::iterator;
using HeaderRange = std::pair<StringViewIterator, StringViewIterator>;
using HeaderRangeOpt = std::optional<std::pair<StringViewIterator, StringViewIterator>>;

constexpr HeaderRangeOpt headerRange(std::string_view content, std::string_view prefix,
                                     std::string_view suffix)
{
	const auto stItr = std::search(content.begin(), content.end(), prefix.begin(), prefix.end());
	if (stItr == content.end()) {
		return std::nullopt;
	}

	auto endItr = std::search(content.begin(), content.end(), suffix.begin(), suffix.end());
	if (endItr == content.end()) {
		return std::nullopt;
	}

	std::advance(endItr, suffix.size());
	return std::pair{stItr, endItr};
}

constexpr std::string_view getHeader(std::string_view content, const auto &headerRange)
{
	const auto headerSize = std::distance(headerRange.first, headerRange.second);
	return content.substr(std::distance(content.begin(), headerRange.first), headerSize);
}

[[maybe_unused]] constexpr std::string_view
findHeader(std::string_view content, std::string_view prefix, std::string_view suffix)
{
	const auto stIt = std::search(content.begin(), content.end(), prefix.begin(), prefix.end());
	auto endIt = std::search(content.begin(), content.end(), suffix.begin(), suffix.end());
	std::advance(endIt, suffix.size());

	const auto headerSize = std::distance(stIt, endIt);
	return content.substr(std::distance(content.begin(), stIt), headerSize);
}

constexpr std::string_view findHeaderBody(std::string_view header, std::string_view prefix,
                                          std::string_view suffix)
{
	auto stBodyIt = header.begin();
	std::advance(stBodyIt, prefix.size());

	auto endBodyIt = header.end();
	std::advance(endBodyIt, -suffix.size());

	auto bodySize = std::distance(stBodyIt, endBodyIt);
	bodySize = std::max(bodySize, 0ll);
	return header.substr(std::distance(header.begin(), stBodyIt), bodySize);
}

std::vector<std::string_view> splitString(std::string_view str, std::string_view delimiter);

std::pair<size_t, size_t> headerLineRange(std::string_view content, const HeaderRange &headerRange);

constexpr auto &getOrDefault(const auto &map, const auto &key, const auto &defaultValue)
{
	const auto itr = map.find(key);
	return itr == map.end() ? defaultValue : itr->second;
}

std::unordered_map<QString, double> collectGitBlameStatistic(
    const GitRepository &repo, const QString &filePath, const std::set<QString> &skipCommits,
    const std::pair<size_t, size_t> &headerLineRange, const AuthorAliasesMap &authorAliases);

struct FilteredAuthors
{
	std::vector<std::pair<QString, double>> authors;
	double share;
};

FilteredAuthors filterAuthors(const std::unordered_map<QString, double> &authors, double share);

std::vector<QString> listGitAuthors(std::unordered_map<QString, double> blameCandidates,
                                    std::unordered_map<QString, double> logCandidates = {});

const std::set<QString> &getBrokenCommits(const GitRepository &repo, bool verbose);

}  // namespace header_helpers