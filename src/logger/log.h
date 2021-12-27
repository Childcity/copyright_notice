#pragma once

#include <QDateTime>
#include <QDebug>

#include "src/constants.h"

#define CN_DEBUG(msg) qDebug() << "[D0]" << msg
#define CN_INF(code, msg) qInfo().nospace() << "[I" << (code) << "]" << ' ' << msg
#define CN_WARN(code, msg) qWarning().nospace() << "[W" << (code) << "]" << ' ' << msg
#define CN_ERR(code, msg) qCritical().nospace() << "[E" << (code) << "]" << ' ' << msg

namespace logger {

// clang-format off
enum MsgCode {
	  Debug                      = 0
	, BadComponentName           = 1
	, BadMaxBlameAuthors         = 2
	, BadTargetPaths             = 3
	, BadStaticConfigPaths       = 4
	, BadStaticConfigFormat      = 5
	, FileOrDirIsNotExist        = 6
	, FileReadWriteError         = 7
	, BadHeaderFormat            = 8
	, RunningExternalToolError   = 9
	, InternalError              = 10
	, GitError                   = 100

	, ProcessingFile             = 500
	, HeaderFound                = 501
	, HeaderNotFound             = 502
	, PossibleAuthors            = 503
	, WouldUpdateCopyrightNotice = 504
	, UpdatedCopyrightNotice     = 505
};
// clang-format on

namespace environment {

void setPattern();

}

void init(bool verbose);

}  // namespace logger
