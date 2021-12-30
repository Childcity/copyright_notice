#include "GitRepository.h"

#include <QFileInfo>
#include <QRegularExpression>

#include "../git_helpers.h"
#include "src/logger/log.h"

namespace {

namespace hlp = git_helpers;

void raiseException(int errorCode, const char *action)
{
	CN_ERR(logger::MsgCode::GitError + errorCode, "Git. Error " << action << '.');
	throw std::exception();
}

QString getBrokenCommit(const QString &line)
{
	// clang-format off
	// https://regexr.com/6ceve
	static const QLatin1String pattern(
	                                   "^"                                           // Line starts.
	                                       "(?P<data>("                                 // To determine that any data exist in line.
	                                                   "?P<commit>[0-9a-f]{40}) "       // Match commit hash (40 characters).
	                                                   "(?P<parent_1>[0-9a-f]{10})"     // Match necessary parent_1 hash (10 characters).
	                                                   "(?P<parent_2> [0-9a-f]{10})? "  // Match not necessary parent_2 hash (10 characters).
	                                                   "(?P<message>(Revert \"|)Merge.+(branch|-( +|)>).+)" // Match merge message.
	                                       ")?"
	                                   "$"                                              // Line ends
	                                  );
	static const QRegularExpression regex(pattern);
	assert(regex.isValid());
	// clang-format on

	const auto match = regex.match(line);
	if (match.hasMatch() && !match.captured("data").isEmpty()) {
		return match.captured("commit");
	}

	return {};
}

}  // namespace

GitRepository::GitRepository(QString repoPath) noexcept
    : m_path(std::move(repoPath))
{}

GitRepository::~GitRepository() = default;

GitRepository::GitRepository(GitRepository &&other) noexcept = default;

void GitRepository::open()
{
	const auto fileDir = QFileInfo(m_path).absolutePath();
	m_workingTreeDir = hlp::runGitTool({"rev-parse", "--show-toplevel"}, fileDir).trimmed();

	if (!m_path.contains(m_workingTreeDir)) {
		raiseException(1, "opening repository");
	}
}

QString GitRepository::getWorkingTreeDir() const
{
	return m_workingTreeDir;
}

std::vector<QString> GitRepository::getBrokenCommits() const
{
	const auto log = hlp::runGitTool({"log", "HEAD", "--pretty=%H %p %s"}, getWorkingTreeDir());
	const auto tokenizedLog = log.split('\n');

	std::vector<QString> result;
	result.reserve(tokenizedLog.size());

	for (const auto &line : tokenizedLog) {
		auto hash = getBrokenCommit(line);
		if (!hash.isEmpty()) {
			result.emplace_back(std::move(hash));
		}
	}

	return result;
}

std::vector<GitBlameLine> GitRepository::blameFile(const QString &filePath) const
{
	return git_helpers::blameFile(getWorkingTreeDir(), filePath);
}
