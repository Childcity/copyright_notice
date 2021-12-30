#pragma once

#include <QString>
#include <unordered_map>

#include "GitBlameLine.h"

namespace git_helpers {

[[nodiscard]] QByteArray runGitTool(const QStringList &arguments, const QString &workingDir = {});
[[nodiscard]] std::vector<GitBlameLine> blameFile(const QString &repoRoot, const QString &filePath);

}  // namespace git_helpers