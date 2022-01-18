#include <QCoreApplication>

#include "configuration/RunConfig.h"
#include "file_processor/FileProcessor.h"
#include "src/logger/log.h"

int main(int argc, char *argv[])
{
	environment::init();
	logger::environment::setPattern();

	QCoreApplication app(argc, argv);
	QCoreApplication::setApplicationName(CN_APP_NAME);
	QCoreApplication::setApplicationVersion(CN_APP_VERSION);

	const RunConfig runConfig(QCoreApplication::arguments());
	logger::init(runConfig.options() & RunOption::Verbose);

	FileProcessor fileProcessor(runConfig);
	fileProcessor.process();

	return fileProcessor.isAnyFileUpdated() ? apperror::FilesChanged : apperror::Success;
}
