#include "log.h"

namespace {

bool gDebugLoggingOn = false;

const QtMessageHandler QT_DEFAULT_MESSAGE_HANDLER = qInstallMessageHandler(nullptr);

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
	if (type == QtDebugMsg && !gDebugLoggingOn) {
		return;
	}

	(*QT_DEFAULT_MESSAGE_HANDLER)(type, context, msg);
}

}  // namespace

void logger::environment::setPattern()
{
	// If a user set QT_MESSAGE_PATTERN globally for system,
	// expected by 'linter util' message pattern will be changed.
	// That is why we need to set QT_MESSAGE_PATTERN explicitly by setting
	// environment variable for this application.
	qputenv("QT_MESSAGE_PATTERN",
	        "%{time}"
	        // " %{file}:%{line}"
	        " %{message}");
}

void logger::init(bool verbose)
{
	gDebugLoggingOn = verbose;
	qInstallMessageHandler(messageHandler);
}
