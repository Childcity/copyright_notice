#pragma once

#include <array>
#include <QString>

namespace appconst {

extern const QLatin1String cAppDescription;
extern const QLatin1String cStaticConfig;
extern const std::array<const QLatin1String, 3> cExcludedPathSections;
constexpr auto cPossibleBrokenCommitsNumber = 1000;
constexpr auto cStartProcessTimeout = 5000;
constexpr auto cProcessExecutionTimeout = 10000;

extern const QLatin1String cCopyrightTemplate;
extern const QLatin1String cEtAl;

namespace json {
extern const QLatin1String cAuthorAliases;
}

}  // namespace appconst

namespace apperror {

// clang-format off
enum ExitCode {
	Success         = 0
	, RunArgError   = 1
	, GitError      = 2
	, ParseError    = 3
	, InternalError = 4
};
// clang-format on

}  // namespace apperror