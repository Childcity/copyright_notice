#pragma once

#include <QFile>

namespace file_utils {

QByteArray readFile(const QString &path);
void writeFile(const QString &path, const QByteArray &content);

}