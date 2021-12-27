#include "header_helpers.h"

#include <mutex>
#include <QRegularExpression>
#include <QStringBuilder>

#include "src/file_processor/git/GitBlameLine.h"
#include "src/logger/log.h"

namespace {

std::unique_ptr<std::set<QString>> gBrokenCommitsInstance;
std::once_flag create;

}  // namespace

namespace header_helpers {

std::vector<std::string_view> splitString(std::string_view str, std::string_view delimiter)
{
	std::vector<std::string_view> strings;

	std::size_t pos;
	std::size_t prev{};
	while ((pos = str.find(delimiter, prev)) != std::string::npos) {
		strings.emplace_back(str.substr(prev, pos - prev));
		prev = pos + 1;
	}

	// To get the last substring (or only, if delimiter is not found)
	strings.emplace_back(str.substr(prev));

	return strings;
}

std::pair<size_t, size_t> headerLineRange(std::string_view content, const HeaderRange &headerRange)
{
	const auto beforeHeaderContent = std::string_view(content.begin(), headerRange.first);
	const auto headerContent = getHeader(content, headerRange);

	const auto beforeHeaderLineCount = splitString(beforeHeaderContent, "\n").size() - 1;
	const auto headerLineCount = splitString(headerContent, "\n").size() - 1;
	return {beforeHeaderLineCount, beforeHeaderLineCount + headerLineCount};
}

std::unordered_map<QString, double> collectGitBlameStatistic(
    const GitRepository &repo, const QString &filePath, const std::set<QString> &skipCommits,
    const std::pair<size_t, size_t> &headerLineRange, const AuthorAliasesMap &authorAliases)
{
	const auto blame = repo.blameFile(filePath);

	const size_t lastHeaderLine = headerLineRange.second;
	auto stItr = blame.cbegin();
	std::advance(stItr, lastHeaderLine);

	std::unordered_map<QString, double> authorScore;
	double blameSum = 0;

	std::for_each(stItr, blame.cend(), [&](const GitBlameLine &line) {
		if (skipCommits.end() != skipCommits.find(line.hash)) {
			CN_DEBUG("Skipping commit " << line.hash);
			return;
		}

		QString key = getOrDefault(authorAliases, line.author, line.author);
		authorScore[key] += 1.0;
		blameSum += 1.0;
	});

	// Normalize values
	for (auto &[key, value] : authorScore) {
		value /= blameSum;
	}

	return authorScore;
}

FilteredAuthors filterAuthors(const std::unordered_map<QString, double> &authors, double share)
{
	auto authorsVec = std::vector<std::pair<QString, double>>(authors.begin(), authors.end());

	std::sort(authorsVec.begin(), authorsVec.end(),
	          [](const auto &l, const auto &r) { return l.second > r.second; });

	double sum = 0;
	for (auto it = authorsVec.begin(); it != authorsVec.end(); it++) {
		sum += it->second;
		if (sum > share) {
			authorsVec.erase(std::next(it), std::end(authorsVec));
			break;
		}
	}

	return {authorsVec, sum};
}

std::vector<QString> listGitAuthors(std::unordered_map<QString, double> blameCandidates,
                                    std::unordered_map<QString, double> logCandidates)
{
	constexpr double etAlTreshold = 8;
	constexpr double etAlMentions = 4;
	constexpr double namesShare = 0.66;
	const double norm = logCandidates.empty() ? 1 : 2;

	auto &&[blameAuthorsVec, blameShare] = filterAuthors(blameCandidates, namesShare);
	auto &&[logAuthorsVec, logShare] = filterAuthors(blameCandidates, namesShare);

	decltype(blameCandidates) allAuthorsDict;
	decltype(blameAuthorsVec) allAuthorsVec;
	allAuthorsVec.insert(allAuthorsVec.end(), blameAuthorsVec.begin(), blameAuthorsVec.end());
	allAuthorsVec.insert(allAuthorsVec.end(), logAuthorsVec.begin(), logAuthorsVec.end());

	for (const auto &[name, score] : allAuthorsVec) {
		allAuthorsDict[name] = score / norm;
	}

	std::vector<QString> result;
	result.reserve(etAlTreshold);

	std::transform(allAuthorsDict.begin(), allAuthorsDict.end(), std::back_inserter(result),
	               [](const auto &p) { return p.first; });

	if (static_cast<double>(allAuthorsDict.size()) < etAlTreshold) {
		std::sort(result.begin(), result.end());
		if ((blameShare + logShare) / norm < namesShare) {
			result.emplace_back(appconst::cEtAl);
		}
		return result;
	}

	std::sort(result.begin(), result.end(), [&allAuthorsDict](const QString &l, const QString &r) {
		return allAuthorsDict.at(l) > allAuthorsDict.at(r);
	});

	auto itr = result.begin();
	std::advance(itr, etAlMentions);

	result.erase(itr, std::end(result));
	std::sort(result.begin(), result.end());
	result.emplace_back(appconst::cEtAl);

	return result;
}

[[maybe_unused]] void logBrokenCommits(const auto &brokenCommits)
{
	QString msg("Will skip the following commits when blaming: ");
	for (const auto &hash : brokenCommits) {
		msg = msg % hash % ", ";
	}
	CN_DEBUG(msg);
}

const std::set<QString> &getBrokenCommits(const GitRepository &repo, bool verbose)
{
	std::call_once(create, [&repo, verbose] {
		auto commitsVec = repo.getBrokenCommits();
		auto commitsSet = std::set<QString>(commitsVec.begin(), commitsVec.end());
		gBrokenCommitsInstance = std::make_unique<std::set<QString>>(std::move(commitsSet));

		if (verbose) {
			// logBrokenCommits(commitsVec);
		}
	});
	return *gBrokenCommitsInstance;
}

}  // namespace header_helpers