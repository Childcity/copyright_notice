#pragma once

#include "git_helpers.h"

#include <QDir>
#include <QProcess>
#include <QRegularExpression>

#include "src/logger/log.h"

namespace {

using Msg = logger::MsgCode;

QByteArray runProgram(const QString &program, const QStringList &arguments,
                      const QString &workingDir = {})
{
	QProcess p;
	p.setWorkingDirectory(workingDir);
	p.start(program, arguments);

	if (!p.waitForStarted(appconst::cStartProcessTimeout)) {
		CN_ERR(Msg::RunningExternalToolError,
		       "Failed to start program " << program << " " << arguments);
		throw std::exception();
	}

	const bool isTimeout = !p.waitForFinished(appconst::cProcessExecutionTimeout);
	if (isTimeout || p.exitCode() != EXIT_SUCCESS) {
		QByteArray errorText = p.readAllStandardError().trimmed();
		if (errorText.isEmpty()) {
			errorText = p.readAllStandardOutput().trimmed();
		}
		CN_ERR(Msg::RunningExternalToolError,
		       "Failed to run program [" << p.exitCode() << "]:" << program << " " << arguments
		                                 << ". Error: " << errorText);
		throw std::exception();
	}

	return p.readAllStandardOutput();
}

void parseBlameLine(const QByteArray &line, std::vector<GitBlameLine> &result)
{
	// clang-format off
	// https://regexr.com/6c4t7
	static const QLatin1String pattern(R"_(^(?P<data>(?P<hash>[0-9a-f]{5,40}) .+ \((?P<author>[\/\\\w]+[\. ]+[\/\\\w]+) .+)?$)_");
	thread_local static QRegularExpression regex(pattern);
	thread_local static QRegularExpressionMatch match;
	// clang-format on

	match = regex.match(line);

	if (!match.hasMatch()) {
		CN_WARN(Msg::RunningExternalToolError,
		        "Git blame returned unexpected line " << line << ".");
		return;
	}

	if (match.captured("data").isEmpty()) {
		return;
	}

	// clang-format off
	result.emplace_back(GitBlameLine{
	    .hash = match.captured("hash"),
	    .author = match.captured("author")
	});
	// clang-format on
}

}  // namespace

namespace git_helpers {

std::vector<GitBlameLine> blameFile(const QString &repoRoot, const QString &filePath)
{
	// clang-format off
	static const QLatin1String program("git");
	const QStringList args = {"blame", "HEAD", "-CC", "-w", "-l", "-f", "-t", "--date=iso", "--", filePath};
	// clang-format on

	CN_DEBUG("Running " << program << args);
	const auto tokenizedBlame = runProgram(program, args, repoRoot).split('\n');

	std::vector<GitBlameLine> result;
	result.reserve(tokenizedBlame.size());

	std::for_each(tokenizedBlame.cbegin(), tokenizedBlame.cend(), [&result](auto &&arg) {
		parseBlameLine(std::forward<decltype(arg)>(arg), result);
	});

	return result;
}

}  // namespace git_helpers