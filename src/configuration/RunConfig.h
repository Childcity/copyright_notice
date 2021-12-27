#pragma once

#include <QStringList>

namespace environment {

void init();

};

// clang-format off
enum class RunOption {
	UpdateComponent            = 1 << 0,
	UpdateCopyright            = 1 << 1,
	UpdateFileName             = 1 << 2,
	UpdateAuthors              = 1 << 3,
	UpdateAuthorsOnlyIfEmpty   = 1 << 4,
	DontSkipBrokenMerges       = 1 << 5,
	ReadOnlyMode               = 1 << 6,
	Verbose                    = 1 << 7
};
Q_DECLARE_FLAGS(RunOptions, RunOption)
// clang-format on

struct RunConfig
{
public:
	explicit RunConfig(const QStringList &arguments) noexcept;

	[[nodiscard]] const RunOptions &options() const { return m_runOptions; }
	[[nodiscard]] const QString &componentName() const { return m_componentName; }
	[[nodiscard]] int maxBlameAuthors() const { return m_maxBlameAuthors; }
	[[nodiscard]] const QString &staticConfigPath() const { return m_staticConfigPath; }
	[[nodiscard]] const QStringList &targetPaths() const { return m_targetPaths; }

	[[nodiscard]] static const struct StaticConfig &getStaticConfig(const QString &path);

private:
	RunOptions m_runOptions;
	QString m_componentName;
	int m_maxBlameAuthors = std::numeric_limits<int>::max();
	QString m_staticConfigPath;
	QStringList m_targetPaths;
};