#include "constants.h"

namespace appconst {

using QLS = const QLatin1String;

QLS cAppDescription(
    "Ensures that files in a project under Git have a consistent copyright notice.");
QLS cStaticConfig("static_config.json");

// clang-format off
const std::array<QLS, 3> cExcludedPathSections = {
    QLS("3rdparty"),
	QLS("TestedDriverFiles"),
	QLS("app/share/ext"),
};
// clang-format on

QLS cCopyrightTemplate("(c) %1, Inc. All Rights Reserved.");
QLS cEtAl("et al.");

namespace json {

QLS cAuthorAliases("author_aliases");

}

}  // namespace appconst