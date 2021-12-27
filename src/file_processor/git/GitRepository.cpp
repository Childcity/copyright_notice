#include "GitRepository.h"

#include <QRegularExpression>

#include <git2.h>

#include "git_helpers.h"
#include "src/logger/log.h"

namespace {

void checkError(int errorCode, const char *action)
{
	using Msg = logger::MsgCode;
	const git_error *error = git_error_last();
	if (!errorCode) {
		return;
	}
	CN_ERR(Msg::GitError - errorCode,
	       "GitLib2. Error " << action << ": "
	                         << ((error && error->message) ? error->message : "unknown") << '.');
	throw std::exception();
}

bool isMergeCommitMessage(const char *messagePtr)
{
	static QRegularExpression re("Merge.+branch.+", QRegularExpression::CaseInsensitiveOption);
	const auto message = QString::fromLatin1(messagePtr);
	return re.match(message).hasMatch();
}

QString getHash(const git_commit &commit)
{
	std::array<char, GIT_OID_HEXSZ + 1> buf{};
	git_oid_tostr(buf.data(), buf.size(), git_commit_id(&commit));
	return QString::fromStdString(buf.data());
}

}  // namespace

GitRepository::GitRepository(QString repoPath) noexcept
    : m_path(std::move(repoPath))
{
	git_libgit2_init();
}

GitRepository::~GitRepository()
{
	git_repository_free(m_repo);
	if (m_needsGitLib2Shutdown) {
		git_libgit2_shutdown();
	}
}

GitRepository::GitRepository(GitRepository &&other) noexcept
    // if we moved from one instance to another, we shouldn't shut down GitLib2
    : m_needsGitLib2Shutdown(std::exchange(other.m_needsGitLib2Shutdown, false))
    , m_path(std::move(other.m_path))
    , m_repo(std::exchange(other.m_repo, nullptr))
{}

void GitRepository::open()
{
	if (m_repo) {
		return;
	}
	auto ec = git_repository_open_ext(&m_repo, m_path.toStdString().c_str(), 0, nullptr);
	checkError(ec, "opening repository");
}

QString GitRepository::getWorkingTreeDir() const
{
	return QString::fromUtf8(git_repository_path(m_repo))
	    .section('/', 0, -2, QString::SectionSkipEmpty);
}

std::vector<QString> GitRepository::getBrokenCommits() const
{
	constexpr int maxParentsCommitsLimit = 2;

	std::vector<QString> result;
	git_revwalk *walker = nullptr;
	git_oid oid;

	auto ec = git_revwalk_new(&walker, m_repo);
	checkError(ec, "could not create revision walker");

	ec = git_revwalk_push_head(walker);
	checkError(ec, "could not find repository HEAD");

	result.reserve(appconst::cPossibleBrokenCommitsNumber);
	for (git_commit *commit{}; !git_revwalk_next(&oid, walker); git_commit_free(commit)) {
		git_commit_lookup(&commit, m_repo, &oid);
		checkError(ec, "failed to look up commit");

		std::size_t parentsCount = git_commit_parentcount(commit);
		if (parentsCount > maxParentsCommitsLimit) {
			continue;
		}

		const auto *message = git_commit_message(commit);
		if (!isMergeCommitMessage(message)) {
			continue;
		}

		result.emplace_back(getHash(*commit));
	}

	git_revwalk_free(walker);
	return result;
}

std::vector<GitBlameLine> GitRepository::blameFile(const QString &filePath) const
{
	return git_helpers::blameFile(getWorkingTreeDir(), filePath);
}
