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

private:
	QString m_path;
	QString m_workingTreeDir;
};
