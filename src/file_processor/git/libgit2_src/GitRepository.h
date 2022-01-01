#pragma once

#include <QString>
#include <vector>

struct GitRepository
{
	explicit GitRepository(QString repoPath) noexcept;
	~GitRepository();
	GitRepository(const GitRepository &) = delete;
	GitRepository(GitRepository &&other) noexcept;

	void open();
	[[nodiscard]] QString getWorkingTreeDir() const;
	[[nodiscard]] std::vector<QString> getBrokenCommits() const;
	[[nodiscard]] std::vector<struct GitBlameLine> blameFile(const QString &filePath) const;

	[[nodiscard]] static QString getWorkingTreeDir(const QString &filePath);

private:
	// LibGit2 must be inited in each thread and shut down when thread is exited
	bool m_needsGitLib2Shutdown = true;

	QString m_path;
	struct git_repository *m_repo = nullptr;
};
