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

bool processFile(Context ctx)
{
	try {
		GitRepository repo(ctx.targetRepoRootPath);
		repo.open();

		CN_INF(Msg::ProcessingFile, "Processing file " << ctx.targetPath << '.');

		const auto content = file_utils::readFile(ctx.targetPath);
		Header header(ctx, content, std::move(repo));
		header.load();

		if (!header.isEmpty()) {
			try {
				header.parse();
				CN_DEBUG("Header found in " << ctx.targetPath << '.');
			} catch (const std::exception &) {
				CN_DEBUG("Header not found or incorrect format met in  " << ctx.targetPath << '.');
			}
		} else {
			CN_DEBUG("Header not found in " << ctx.targetPath << '.');
		}

		if (!header.fix()) {
			CN_DEBUG("Header in file" << ctx.targetPath << "will not be updated.");
			return false;
		}

		CN_DEBUG("Header in file" << ctx.targetPath << "needs to be updated.");

		const auto headerData = header.serialize();

		if (ctx.config.options() & RunOption::ReadOnlyMode) {
			// clang-format off
			CN_INF(Msg::WouldUpdateCopyrightNotice,
					"Would update Copyright Notice in file " << ctx.targetPath
					<< " with the following:\n" << headerData);
			// clang-format on
			return false;
		}

		const auto contentData = header.contentWithoutHeader();
		file_utils::writeFile(ctx.targetPath, headerData + contentData);

		CN_INF(Msg::UpdatedCopyrightNotice,
		       "Updated Copyright Notice in file: " << ctx.targetPath << '.');

	} catch (const std::exception &ex) {
		CN_ERR(Msg::InternalError,
		       "Cannot process file " << ctx.targetPath << ": " << ex.what() << '.');
		return false;
	}

	return true;
}

}  // namespace

void FileProcessor::process()
{
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
	QString gitRepoRoot;

	try {
		gitRepoRoot = GitRepository::getWorkingTreeDir(targetPath);
		CN_DEBUG("Using repository" << gitRepoRoot);
	} catch (const std::exception &ex) {
		CN_ERR(Msg::BadTargetPaths,
		       "Cannot process file or dir " << targetPath << " : " << ex.what() << '.');
		return;
	}

	const QFileInfo target(targetPath);
	const auto &staticConfig = getStaticConfig(m_config);

	if (target.isFile()) {
		if (!targetPath.contains(gitRepoRoot)) {
			CN_WARN(Msg::FileOutsideOfRepository,
			        "Skip file " << targetPath << "that is outside of repo " << gitRepoRoot);
			return;
		}

		if (isPathExcluded(targetPath, staticConfig.excludedPathSections())) {
			CN_DEBUG("Skip excluded file" << targetPath);
			return;
		}

		processFile({targetPath, std::move(gitRepoRoot), m_config});
		return;
	}

	signal(SIGABRT, onTermination);
	signal(SIGINT, onTermination);
	signal(SIGTERM, onTermination);  // *UNIX only

	QDirIterator it(targetPath, QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);
	while (it.hasNext()) {
		auto filePath = it.next();

		if (!filePath.contains(gitRepoRoot)) {
			CN_WARN(Msg::FileOutsideOfRepository,
			        "Skip file or dir " << filePath << " that is outside of repo " << gitRepoRoot);
			continue;
		}

		if (isPathExcluded(filePath, staticConfig.excludedPathSections())) {
			CN_DEBUG("Skip excluded file or dir" << filePath);
			continue;
		}

		gThreadPool.start([this, filePath, gitRepoRoot]() mutable {
			if (processFile({std::move(filePath), std::move(gitRepoRoot), m_config})) {
				m_isAnyFileUpdated.test_and_set(std::memory_order_relaxed);
			}
		});
	}

	std::lock_guard l(gThreadPoolMutex);
	gThreadPool.waitForDone();
}

bool FileProcessor::isAnyFileUpdated()
{
	return m_isAnyFileUpdated.test();
}
