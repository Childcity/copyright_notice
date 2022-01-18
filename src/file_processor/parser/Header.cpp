#include "Header.h"

#include <QStringBuilder>

#include "src/configuration/StaticConfig.h"
#include "src/logger/log.h"

#include "header_utils.h"

namespace {

using Msg = logger::MsgCode;
using HeaderFieldsVec = std::vector<std::pair<Header::HeaderFieldType, std::any>>;

void printPossibleAuthors(const auto &ctx, const auto &authors)
{
	QString authorsStr;
	for (const auto &author : authors) {
		authorsStr = authorsStr % author % ", ";
	}
	authorsStr.chop(2);
	CN_INF(Msg::PossibleAuthors,
	       "Detected more than "
	           << ctx.config.maxBlameAuthors() << " authors in the file " << ctx.targetPath
	           << ". Script did NOT update the authors fields! The authors might be: "
	           << authorsStr);
}

int maxAlignedFieldNameLength(const HeaderFieldsVec &fields)
{
	int max = 0;
	for (const auto &[type, _] : fields) {
		if (type != Header::HeaderFieldType::Component) {
			max = std::max(max, header_fields::toFieldName(type).size());
		}
	}
	return max;
}

const StaticConfig &getStaticConfig(const auto &ctx)
{
	const auto &path = ctx.config.staticConfigPath();
	return RunConfig::getStaticConfig(path);
}

}  // namespace

Header::Header(const Context &ctx, const QByteArray &content, GitRepository repo) noexcept
    : m_ctx(ctx)
    , m_content(content)
    , m_repo(std::move(repo))
{
	m_ext = ctx.targetPath.section('.', -1, -1).toStdString();
}

void Header::load()
{
	initRegex();

	m_headerRangeOpt = header_helpers::headerRange(m_content.data(), m_prefix, m_suffix);
	if (!m_headerRangeOpt.has_value()) {
		return;
	}

	const auto headerRange = m_headerRangeOpt.value();
	m_rawHeader = header_helpers::getHeader(m_content.data(), headerRange);
}

void Header::parse()
{
	const auto body = header_helpers::findHeaderBody(m_rawHeader, m_prefix, m_suffix);
	if (body.empty()) {
		return;
	}

	try {
		const auto tokenizedBody = header_helpers::splitString(body, "\n");
		std::for_each(tokenizedBody.cbegin(), tokenizedBody.cend(),
		              [this](auto &&arg) { parseField(std::forward<decltype(arg)>(arg)); });
	} catch (const std::exception &) {
		m_headerRangeOpt = std::nullopt;
		m_rawHeader = {};
		throw;
	}
}

bool Header::fix()
{
	bool hasChanges = false;

	if (m_ctx.config.options() & RunOption::UpdateFileName) {
		auto fileName = m_ctx.targetPath.mid(m_ctx.targetPath.lastIndexOf("/") + 1);
		hasChanges |= fixField(HeaderFieldType::File, std::move(fileName));
	}

	if (m_ctx.config.options() & RunOption::UpdateCopyright) {
		const auto &copyrightTemplate = getStaticConfig(m_ctx).copyrightFieldTemplate();
		static const auto copyrightValue = header_fields::makeCopyrightValue(copyrightTemplate);
		hasChanges |= fixField(HeaderFieldType::Copyright, copyrightValue);
	}

	if (m_ctx.config.options() & RunOption::UpdateComponent) {
		const auto &componentName = m_ctx.config.componentName();
		if (componentName.isEmpty()) {
			m_fields.erase(HeaderFieldType::Component);
			hasChanges = true;
		} else {
			hasChanges |= fixField(HeaderFieldType::Component, componentName);
		}
	}

	bool authorsUpdated = false;
	if (m_ctx.config.options() & RunOption::UpdateAuthors) {
		std::vector<QString> authors;

		if (mayUpdateAuthors()) {
			authors = getAuthors();

			if (authors.size() <= m_ctx.config.maxBlameAuthors()) {
				hasChanges |= fixField(HeaderFieldType::Author, std::move(authors));
				authorsUpdated = hasChanges;
			} else {
				printPossibleAuthors(m_ctx, authors);
			}
		}
	}

	if (!authorsUpdated) {
		CN_DEBUG("Skip author field updates.");
	}

	return hasChanges;
}

QByteArray Header::serialize() const
{
	namespace flds = header_fields;

	auto fields = HeaderFieldsVec(m_fields.begin(), m_fields.end());
	std::sort(fields.begin(), fields.end(),
	          [](const auto &l, const auto &r) { return l.first < r.first; });

	const int maxFieldNameLength = maxAlignedFieldNameLength(fields);

	QByteArray result;
	QTextStream stream(&result);
	const auto serializeLine = [&](HeaderFieldType type, const QString &value) {
		if (type == HeaderFieldType::Component) {
			stream << m_start.data() << '\n';
		}

		stream << m_start.data() << ' ' << qSetFieldWidth(maxFieldNameLength) << Qt::left
		       << flds::toFieldName(type).toLatin1() << qSetFieldWidth(0) << ' ' << value;

		if (flds::requiresFullstop(type)) {
			stream << '.';
		}

		stream << '\n';
	};

	stream << m_prefix.data();
	for (const auto &[type, valueAny] : fields) {
		if (valueAny.type() == typeid(FieldList)) {
			const auto &value = any_cast<const FieldList &>(valueAny);
			std::for_each(value.cbegin(), value.cend(), [type, &serializeLine](const auto &value) {
				serializeLine(type, value);
			});
			continue;
		}

		const auto &value = any_cast<const FieldValue &>(valueAny);
		serializeLine(type, value);
	}
	stream << Qt::flush;

	// Replace last '\n' character to the suffix (result will automatically resize to proper size).
	result.replace(result.size() - 1, static_cast<int>(m_suffix.size()), m_suffix.data());
	return result;
}

QByteArray Header::contentWithoutHeader() const
{
	if (!m_headerRangeOpt.has_value()) {
		return m_content;
	}

	const auto contentView = std::string_view(m_content.data());
	const auto headerRange = m_headerRangeOpt.value();
	const auto headerEndDistance = std::distance(contentView.begin(), headerRange.second);
	return m_content.mid(static_cast<int>(headerEndDistance));
}

void Header::initRegex()
{
	m_prefix = header_utils::prefixMap[m_ext];
	m_start = header_utils::startMap[m_ext];
	m_suffix = header_utils::suffixMap[m_ext];

	// clang-format off
    static const QLatin1String pattern("^" "%1" "(?P<fieldData> (?P<fieldName>" "%2" ") +(?P<fieldValue>.*))?$");
	// clang-format on

	const auto start = QString::fromLatin1(m_start.data());
	m_regex.setPattern(
	    pattern.arg(QRegularExpression::escape(start), header_fields::allWithSeparator));
	assert(m_regex.isValid());
}

void Header::parseField(std::string_view rawField)
{
	if (rawField.empty() || rawField == m_start) {
		return;
	}

	const auto qRawField = QString::fromUtf8(rawField.data(), static_cast<int>(rawField.size()));
	const auto match = m_regex.match(qRawField);

	if (!match.hasMatch()) {
		CN_ERR(Msg::BadHeaderFormat, "Error matching header for " << m_ctx.targetPath << '.');
		throw std::exception();
	}

	if (match.captured("fieldData").isEmpty()) {
		return;
	}

	const auto fieldName = match.captured("fieldName");
	const auto fieldType = header_fields::toFieldType(fieldName);
	auto fieldValue = match.captured("fieldValue");

	if (header_fields::requiresFullstop(fieldType) && fieldValue.endsWith('.')) {
		fieldValue.chop(1);
	}

	const bool isFieldExist = m_fields.contains(fieldType);
	if (header_fields::isListField(fieldType)) {
		if (!isFieldExist) {
			m_fields[fieldType] = FieldList{fieldValue};
		} else {
			any_cast<FieldList &>(m_fields[fieldType]).emplace_back(std::move(fieldValue));
		}
	} else {
		if (isFieldExist) {
			CN_DEBUG(header_fields::toFieldName(fieldType)
			         << "field was met again in the same header in" << m_ctx.targetPath);
		}
		m_fields[fieldType] = std::move(fieldValue);
	}
}

bool Header::fixField(HeaderFieldType name, std::any valueAny)
{
	const bool shouldFix = valueAny.type() == typeid(FieldList)
	    ? shouldFixListField(name, any_cast<const FieldList &>(valueAny))
	    : shouldFixValueField(name, any_cast<const FieldValue &>(valueAny));

	if (!shouldFix) {
		return false;
	}

	m_fields[name] = std::move(valueAny);
	return true;
}

bool Header::shouldFixListField(HeaderFieldType type, const Header::FieldList &value) const
{
	if (m_fields.contains(type)) {
		const auto &fieldList = any_cast<const FieldList &>(m_fields.at(type));
		if (std::equal(value.begin(), value.end(), fieldList.begin(), fieldList.end())) {
			return false;
		}
	}
	return true;
}

bool Header::shouldFixValueField(HeaderFieldType type, const Header::FieldValue &value) const
{
	if (m_fields.contains(type)) {
		const auto &fieldValue = any_cast<const FieldValue &>(m_fields.at(type));
		if (value == fieldValue) {
			return false;
		}
	}
	return true;
}

bool Header::mayUpdateAuthors() const
{
	bool isAuthorFieldExist = false;
	if (m_fields.contains(HeaderFieldType::Author)) {
		const auto &authors = any_cast<const FieldList &>(m_fields.at(HeaderFieldType::Author));
		isAuthorFieldExist = !authors.empty();
	}
	const bool mustUpdateOnlyIfEmpty =
	    m_ctx.config.options().testFlag(RunOption::UpdateAuthorsOnlyIfEmpty);
	return !(mustUpdateOnlyIfEmpty && isAuthorFieldExist);
}

std::vector<QString> Header::getAuthors()
{
	namespace hlp = header_helpers;
	static const auto emptySet = std::set<QString>();

	const bool verbose = m_ctx.config.options() & RunOption::Verbose;
	const bool skipBrokenCommit = !m_ctx.config.options().testFlag(RunOption::DontSkipBrokenMerges);

	const auto &authorAliases = getStaticConfig(m_ctx).authorAliases();
	const auto &brokenCommits =
	    skipBrokenCommit ? hlp::getBrokenCommits(m_repo, verbose) : emptySet;

	const auto headerLineRange = m_headerRangeOpt.has_value()
	    ? hlp::headerLineRange(m_content.data(), m_headerRangeOpt.value())
	    : std::pair(0ull, 0ull);

	auto candidates = hlp::collectGitBlameStatistic(m_repo, m_ctx.targetPath, brokenCommits,
	                                                headerLineRange, authorAliases);
	return hlp::listGitAuthors(std::move(candidates));
}
