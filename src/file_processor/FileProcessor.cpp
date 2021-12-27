#include "FileProcessor.h"

#include <QDirIterator>
#include <QStringBuilder>
#include <QThreadPool>

#include <csignal>

#include "src/file_processor/parser/Header.h"
#include "src/file_processor/parser/header_utils.h"
#include "src/file_utils/file_utils.h"
#include "src/logger/log.h"

namespace {

using Msg = logger::MsgCode;

QThreadPool gThreadPool;
std::mutex gThreadPoolMutex;

void onTermination(int)
{
	std::lock_guard l(gThreadPoolMutex);
	gThreadPool.clear();
}

const StaticConfig &getStaticConfig(const RunConfig &config)
{
	const auto &path = config.staticConfigPath();
	return RunConfig::getStaticConfig(path);
}

constexpr bool isExtensionExcluded(const QString &path)
{
	using namespace appconst;
	return std::none_of(header_utils::prefixMap.cbegin(), header_utils::prefixMap.cend(),
	                    [&path](const auto &p) {
		                    const auto ext = path.section('.', -1, -1);
		                    return p.first == ext.toStdString();
	                    });
}

bool isPathExcluded(const QString &path, const ExcludedPathSections &excluded)
{
	using namespace appconst;
	return std::any_of(excluded.cbegin(), excluded.cend(),
	                   [&path](const auto &p) { return path.contains(p); })
	    || isExtensionExcluded(path);
}

void processFile(Context ctx)
{
	try {
		GitRepository repo(ctx.targetPath);
		repo.open();

		CN_INF(Msg::ProcessingFile, "Processing file " << ctx.targetPath << '.');

		const auto content = file_utils::readFile(ctx.targetPath);
		Header header(ctx, content, std::move(repo));
		header.load();

		if (!header.isEmpty()) {
			header.parse();
			CN_DEBUG("Header found in " << ctx.targetPath << '.');
		} else {
			CN_DEBUG("Header not found in " << ctx.targetPath << '.');
		}

		if (!header.fix()) {
			CN_DEBUG("Header in file" << ctx.targetPath << "will not be updated.");
			return;
		}

		CN_DEBUG("Header in file" << ctx.targetPath << "needs to be updated.");

		const auto headerData = header.serialize();

		if (ctx.config.options() & RunOption::ReadOnlyMode) {
			// clang-format off
			CN_INF(Msg::WouldUpdateCopyrightNotice,
					"Would update Copyright Notice in file " << ctx.targetPath
					<< " with the following:\n" << headerData);
			// clang-format on
			return;
		}

		const auto contentData = header.contentWithoutHeader();
		file_utils::writeFile(ctx.targetPath, headerData + contentData);

		CN_INF(Msg::UpdatedCopyrightNotice,
		       "Updated Copyright Notice in file: " << ctx.targetPath << '.');

	} catch (const std::exception &ex) {
		CN_ERR(Msg::InternalError,
		       "Cannot process file " << ctx.targetPath << ": " << ex.what() << '.');
		return;
	}
}

}  // namespace

void FileProcessor::process()
{
	try {
		GitRepository repo(m_config.targetPaths().first());
		repo.open();

		CN_DEBUG("Using repository" << repo.getWorkingTreeDir());
	} catch (const std::exception &ex) {
		CN_ERR(Msg::InternalError,
		       "Cannot process file " << m_config.targetPaths().first() << ": " << ex.what());
		return;
	}

	for (const auto &path : m_config.targetPaths()) {
		if (!QFileInfo::exists(path)) {
			CN_WARN(Msg::FileOrDirIsNotExist, "Skip not existed target " << path);
			continue;
		}

		process(path);
	}
}

void FileProcessor::process(const QString &targetPath)
{
	const QFileInfo target(targetPath);
	const auto &staticConfig = getStaticConfig(m_config);

	if (target.isFile()) {
		if (isPathExcluded(targetPath, staticConfig.excludedPathSections())) {
			CN_DEBUG("Skip excluded file" << targetPath);
			return;
		}

		processFile({targetPath, m_config});
		return;
	}

	signal(SIGABRT, onTermination);
	signal(SIGINT, onTermination);
	signal(SIGTERM, onTermination);  // *UNIX only

	QDirIterator it(targetPath, QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);
	while (it.hasNext()) {
		auto filePath = it.next();

		if (isPathExcluded(filePath, staticConfig.excludedPathSections())) {
			CN_DEBUG("Skip excluded file or dir" << filePath);
			continue;
		}

		gThreadPool.start([this, filePath] { processFile({filePath, m_config}); });
	}

	std::lock_guard l(gThreadPoolMutex);
	gThreadPool.waitForDone();
}
