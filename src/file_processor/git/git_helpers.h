#pragma once

#include <QString>
#include <unordered_map>

#include "GitBlameLine.h"

namespace git_helpers {

[[nodiscard]] std::vector<GitBlameLine> blameFile(const QString &repoRoot, const QString &filePath);

}  // namespace git_helpers