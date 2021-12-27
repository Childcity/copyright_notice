#include "file_utils.h"

#include <QFile>

#include "src/logger/log.h"

namespace file_utils {

QByteArray readFile(const QString &path)
{
	using Msg = logger::MsgCode;

	QFile file(path);
	if (!file.open(QIODevice::ReadOnly | QIODevice::ExistingOnly | QIODevice::Text)) {
		CN_ERR(Msg::FileReadWriteError,
		       "Error opening file " << path << ": " << file.errorString());
		throw std::exception();
	}

	QByteArray content = file.readAll();
	if (content.isEmpty()) {
		CN_ERR(Msg::FileReadWriteError, "Error reading file or file is empty " << path);
		throw std::exception();
	}

	return content;
}

void writeFile(const QString &path, const QByteArray &content)
{
	using Msg = logger::MsgCode;

	QFile file(path);
	if (!file.open(QIODevice::WriteOnly | QIODevice::ExistingOnly | QIODevice::Text)) {
		CN_ERR(Msg::FileReadWriteError,
		       "Error opening file " << path << ": " << file.errorString());
		throw std::exception();
	}

	const auto written = file.write(content);
	if (written != content.size()) {
		CN_ERR(Msg::FileReadWriteError,
		       "Error writing file " << path << ": " << file.errorString());
		throw std::exception();
	}
}

}  // namespace file_utils