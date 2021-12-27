#include "constants.h"

namespace appconst {

using QLS = const QLatin1String;

QLS cAppDescription(
    "Ensures that files in a project under Git have a consistent copyright notice.");
QLS cStaticConfig("static_config.json");

QLS cEtAl("et al.");

namespace json {

QLS cAuthorAliases("author_aliases");
QLS cCopyrightFieldTemplate("copyright_field_template");
QLS cExcludedPathSections("excluded_path_sections");

}

}  // namespace appconst