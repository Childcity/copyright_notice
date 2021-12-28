#include <QtTest>

#include <QStringList>

#include "../src/configuration/RunConfig.h"
#include "../src/logger/log.h"

class RunConfigTest : public QObject
{
	Q_OBJECT

private slots:
	void initTestCase();
	void test_AllArguments();
	void test_ReadOnlyMode();
	void test_MaxBlameAuthors();
};

void RunConfigTest::initTestCase()
{
	logger::environment::setPattern();
}

void RunConfigTest::test_AllArguments()
{
	const QString expctComponent = "Incorporation Inc.";
	const QString expctMaxBlameAuthors = "2";
	const QString expctStaticConfPath = "/not/existed/file/conf.json";
	const QStringList expctTargets = {"/not/existed/dir", "/not/existed/file.h"};

	// clang-format off
	const QStringList args = {
	    QCoreApplication::applicationFilePath()
	    , "--component", expctComponent
	    , "--update-copyright"
	    , "--update-filename"
	    , "--update-authors"
	    , "--update-authors-only-if-empty"
		, "--max-blame-authors-to-start-update", expctMaxBlameAuthors
	    , "--dont-skip-broken-merges"
	    , "--static-config", expctStaticConfPath
	    , "--dry"
	    , "--verbose"
		, expctTargets.first(), expctTargets.last()
	};
	// clang-format on

	QCOMPARE(args.size(), 16);

	const RunConfig runConfig(args);

	QVERIFY(runConfig.options() & RunOption::UpdateComponent);
	QVERIFY(runConfig.options() & RunOption::UpdateCopyright);
	QVERIFY(runConfig.options() & RunOption::UpdateFileName);
	QVERIFY(runConfig.options() & RunOption::UpdateAuthors);
	QVERIFY(runConfig.options() & RunOption::UpdateAuthorsOnlyIfEmpty);
	QVERIFY(runConfig.options() & RunOption::DontSkipBrokenMerges);
	QVERIFY(runConfig.options() & RunOption::ReadOnlyMode);
	QVERIFY(runConfig.options() & RunOption::Verbose);

	QVERIFY(!runConfig.componentName().isEmpty());
	QCOMPARE(runConfig.componentName(), expctComponent);

	QCOMPARE(runConfig.maxBlameAuthors(), expctMaxBlameAuthors.toInt());

	QVERIFY(!runConfig.staticConfigPath().isEmpty());
	QCOMPARE(runConfig.staticConfigPath(), expctStaticConfPath);

	QVERIFY(!runConfig.targetPaths().isEmpty());
	QCOMPARE(runConfig.targetPaths(), expctTargets);
}

void RunConfigTest::test_ReadOnlyMode()
{
	// clang-format off
	const QStringList args = {
	    QCoreApplication::applicationFilePath()
		// , "--dry" -- is OFF for test
		, "/not/used/for/test"
	};
	// clang-format on

	for (auto &v : {"False", "false", "F", "f", "0"}) {
		qputenv("LINT_ENABLE_COPYRIGHT_UPDATE", v);
		const RunConfig runConfig(args);
		QVERIFY2(runConfig.options() & RunOption::ReadOnlyMode,
		         qPrintable(QString("Failed with tested value '%1'").arg(v)));
	}

	qputenv("LINT_ENABLE_COPYRIGHT_UPDATE", "");
	const RunConfig runConfig2(args);
	QVERIFY(!runConfig2.options().testFlag(RunOption::ReadOnlyMode));
}

void RunConfigTest::test_MaxBlameAuthors()
{
	constexpr auto maxInt = std::numeric_limits<int>::max();

	// clang-format off
	const QStringList args = {
	    QCoreApplication::applicationFilePath()
		, "/template/"
		, "/not/used/for/test"
	};
	// clang-format on

	{
		const RunConfig runConfig(args);
		QCOMPARE(runConfig.maxBlameAuthors(), maxInt);
	}

	{
		auto zeroBlameAuthorsArgs = args;
		zeroBlameAuthorsArgs.replace(1, "--max-blame-authors-to-start-update=0");
		const RunConfig zeroRunConfig(zeroBlameAuthorsArgs);
		QCOMPARE(zeroRunConfig.maxBlameAuthors(), maxInt);
	}

	{
		auto negativeBlameAuthorsArgs = args;
		negativeBlameAuthorsArgs.replace(1, "--max-blame-authors-to-start-update=-1");
		const RunConfig negativeRunConfig(negativeBlameAuthorsArgs);
		QCOMPARE(negativeRunConfig.maxBlameAuthors(), maxInt);
	}

	{
		auto oneBlameAuthorsArgs = args;
		oneBlameAuthorsArgs.replace(1, "--max-blame-authors-to-start-update=1");
		const RunConfig oneRunConfig(oneBlameAuthorsArgs);
		QCOMPARE(oneRunConfig.maxBlameAuthors(), 1);
	}
}

QTEST_GUILESS_MAIN(RunConfigTest)

#include "tst_RunConfigTest.moc"