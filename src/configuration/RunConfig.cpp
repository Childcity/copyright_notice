#include "RunConfig.h"

#include <mutex>
#include <QCommandLineParser>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "src/file_utils/file_utils.h"
#include "src/logger/log.h"
#include "StaticConfig.h"

namespace environment {

void init()
{
	qputenv("QT_ENABLE_REGEXP_JIT", "1");
}

bool copyrightUpdateNotAllowed()
{
	static constexpr const char *falseValues[]{"False", "false", "F", "f", "0"};
	const auto enabled = qgetenv("LINT_ENABLE_COPYRIGHT_UPDATE");
	return std::any_of(std::cbegin(falseValues), std::cend(falseValues),
	                   [&enabled](const auto &fv) { return enabled == fv; });
}

}  // namespace environment

namespace {

using Msg = logger::MsgCode;

// clang-format off
QCommandLineOption componentName{"component", "Add or replace software component mention.", "name"};
QCommandLineOption updateCopyright{"update-copyright", "Add or update copyright field (Year, Company, etc...)."};
QCommandLineOption updateFileName{"update-filename", "Add or fix 'File' field."};

QCommandLineOption updateAuthors{"update-authors", "Add or update author list."};
QCommandLineOption updateAuthorsOnlyIfEmpty{
    "update-authors-only-if-empty",
	"Update author list only if this list is empty in author field (edited by someone else)."};
QCommandLineOption maxBlameAuthors{
    "max-blame-authors-to-start-update",
	"Update author list only if blame authors <= some limit.", "number", "0"};

QCommandLineOption dontSkipBrokenMerges{
    "dont-skip-broken-merges", "Do not skip broken merge commits."};

QCommandLineOption staticConfigPath{
    "static-config", "Json configuration file with static configuration.", "path"};
QCommandLineOption dry{"dry", "Do not modify files, print to stdout instead."};
QCommandLineOption verbose{"verbose", "Print verbose output."};
// clang-format on

void addOptions(QCommandLineParser &parser)
{
	parser.setApplicationDescription(appconst::cAppDescription);
	parser.addHelpOption();
	parser.addVersionOption();

	// clang-format off
	parser.addOptions({
	    componentName
	    , updateCopyright
	    , updateFileName
	    , updateAuthors
	    , updateAuthorsOnlyIfEmpty
		, maxBlameAuthors
	    , dontSkipBrokenMerges
	    , staticConfigPath
	    , dry
	    , verbose
	});
	// clang-format on

	parser.addPositionalArgument("file_or_dir", "File or directory to process.", "[paths...]");
}

}  // namespace

RunConfig::RunConfig(const QStringList &arguments) noexcept
{
	QCommandLineParser parser;
	addOptions(parser);
	parser.process(arguments);

	if (parser.isSet(verbose)) {
		m_runOptions |= RunOption::Verbose;
	}

	if (parser.isSet(::componentName)) {
		m_runOptions |= RunOption::UpdateComponent;
		m_componentName = parser.value(::componentName);
		if (m_componentName.isEmpty()) {
			CN_WARN(Msg::BadComponentName,
			        "Component name is empty, that is why this field will be deleted.");
		}
	}

	if (parser.isSet(updateCopyright)) {
		m_runOptions |= RunOption::UpdateCopyright;
	}

	if (parser.isSet(updateFileName)) {
		m_runOptions |= RunOption::UpdateFileName;
	}

	if (parser.isSet(updateAuthors) && qgetenv("LINT_ENABLE_COPYRIGHT_UPDATE") == "") {
		m_runOptions |= RunOption::UpdateAuthors;
	}

	if (parser.isSet(updateAuthorsOnlyIfEmpty)) {
		m_runOptions |= RunOption::UpdateAuthorsOnlyIfEmpty;
	}

	if (parser.isSet(::maxBlameAuthors)) {
		bool isOk{};
		m_maxBlameAuthors = parser.value(::maxBlameAuthors).toInt(&isOk);
		if (!isOk) {
			CN_ERR(Msg::BadMaxBlameAuthors,
			       ::maxBlameAuthors.names().first() << " should be a positive number (0 or -1 "
			                                            "mean 'unlimited' and used by default).");
			std::exit(apperror::RunArgError);
		}

		constexpr auto maxInt = std::numeric_limits<int>::max();
		m_maxBlameAuthors = m_maxBlameAuthors > 0 ? m_maxBlameAuthors : maxInt;
	}

	if (parser.isSet(dontSkipBrokenMerges)) {
		m_runOptions |= RunOption::DontSkipBrokenMerges;
	}

	if (parser.isSet(::staticConfigPath)) {
		m_staticConfigPath = parser.value(::staticConfigPath);
		if (m_staticConfigPath.isEmpty()) {
			CN_ERR(Msg::BadStaticConfigPaths,
			       ::staticConfigPath.names().first() << " should not be empty string.");
			std::exit(apperror::RunArgError);
		}
	} else {
		QDir appDir(qApp->applicationDirPath());
		m_staticConfigPath = appDir.canonicalPath() + QDir::separator() + appconst::cStaticConfig;
	}

	m_staticConfigPath = QDir::cleanPath(m_staticConfigPath);

	if (m_runOptions & RunOption::Verbose) {
		CN_DEBUG("Using static-config path " << m_staticConfigPath);
	}

	if (parser.isSet(dry) || environment::copyrightUpdateNotAllowed()) {
		m_runOptions |= RunOption::ReadOnlyMode;
	}

	m_targetPaths = parser.positionalArguments();
	if (m_targetPaths.isEmpty() || m_targetPaths.first().isEmpty()) {
		CN_ERR(Msg::BadTargetPaths, "'file_or_dir' should not be empty string.");
		std::exit(apperror::RunArgError);
	}

	std::transform(m_targetPaths.cbegin(), m_targetPaths.cend(), m_targetPaths.begin(),
	               [](const auto &path) { return QDir::cleanPath(path); });
}

namespace impl {

StaticConfig getStaticConfig(const QString &path)
{
	using namespace appconst::json;

	const auto handleError = [&path](const auto &action) {
		CN_ERR(Msg::BadStaticConfigFormat,
		       QString("Error parsing static config '%1': %2").arg(path, action));
		std::exit(apperror::RunArgError);
	};

	const auto content = file_utils::readFile(path);

	const auto doc = QJsonDocument::fromJson(content);
	const auto root = doc.object();

	if (root.isEmpty()) {
		handleError("root object is empty");
		return {};
	}

	const auto aliasesVal = root.value(cAuthorAliases);
	if (!root.contains(cAuthorAliases) || !aliasesVal.isObject()) {
		handleError(QString("map '%1' not found").arg(cAuthorAliases));
		return {};
	}

	AuthorAliasesMap authorAliases;
	const auto aliasesVariantHash = aliasesVal.toObject().toVariantHash();
	std::for_each(aliasesVariantHash.constKeyValueBegin(), aliasesVariantHash.constKeyValueEnd(),
	              [&authorAliases](const auto &keyValue) {
		              authorAliases[keyValue.first] = keyValue.second.toString();
	              });

	const auto copyrightFieldTemplateVal = root.value(cCopyrightFieldTemplate);
	if (!root.contains(cCopyrightFieldTemplate) || !copyrightFieldTemplateVal.isString()) {
		handleError(QString("string '%1' not found").arg(cCopyrightFieldTemplate));
		return {};
	}

	const auto excludedPathSectionsVal = root.value(cExcludedPathSections);
	if (!root.contains(cExcludedPathSections) || !excludedPathSectionsVal.isArray()) {
		handleError(QString("array '%1' not found").arg(cExcludedPathSections));
		return {};
	}

	ExcludedPathSections excludedPathSections;
	const auto excludedPathSectionsArr = excludedPathSectionsVal.toArray();
	std::transform(excludedPathSectionsArr.cbegin(), excludedPathSectionsArr.cend(),
	               std::back_inserter(excludedPathSections),
	               [](const auto &value) { return value.toString(); });

	// clang-format off
	return {
	    std::move(authorAliases),
	    copyrightFieldTemplateVal.toString(),
	    std::move(excludedPathSections)
	};
	// clang-format on
}

static std::unique_ptr<StaticConfig> gStaticConfigInstance;
static std::once_flag create;

}  // namespace impl

const StaticConfig &RunConfig::getStaticConfig(const QString &path)
{
	std::call_once(impl::create, [=] {
		impl::gStaticConfigInstance = std::make_unique<StaticConfig>(impl::getStaticConfig(path));
	});
	return *::impl::gStaticConfigInstance;
}
