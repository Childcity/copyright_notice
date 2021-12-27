#pragma once

#include <any>
#include <QRegularExpression>
#include <QString>
#include <vector>

#include "header_fields.h"
#include "header_helpers.h"
#include "src/file_processor/Context.h"
#include "src/file_processor/git/GitRepository.h"

struct Header
{
	using FieldValue = QString;
	using FieldList = std::vector<FieldValue>;
	using HeaderFieldType = header_fields::FieldType;

	Header(const Context &ctx, const QByteArray &content, GitRepository repo) noexcept;

	void load();
	void parse();
	bool fix();
	[[nodiscard]] QByteArray serialize() const;
	[[nodiscard]] QByteArray contentWithoutHeader() const;

	constexpr bool isEmpty() noexcept { return m_rawHeader.empty(); }

private:
	void initRegex();
	void parseField(std::string_view rawField);
	bool fixField(HeaderFieldType type, std::any valueAny);
	[[nodiscard]] bool shouldFixListField(HeaderFieldType type, const FieldList &value) const;
	[[nodiscard]] bool shouldFixValueField(HeaderFieldType type, const FieldValue &value) const;
	[[nodiscard]] bool mayUpdateAuthors() const;
	[[nodiscard]] std::vector<QString> getAuthors();

private:
	const Context &m_ctx;
	const QByteArray &m_content;
	GitRepository m_repo;
	QRegularExpression m_regex;
	std::string m_ext;
	std::string_view m_prefix;
	std::string_view m_start;
	std::string_view m_suffix;

	std::string_view m_rawHeader;
	header_helpers::HeaderRangeOpt m_headerRangeOpt;
	std::unordered_map<HeaderFieldType, std::any> m_fields;
};
