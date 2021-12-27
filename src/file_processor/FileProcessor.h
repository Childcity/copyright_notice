#pragma once

#include "Context.h"
#include "src/file_processor/git/GitRepository.h"

struct FileProcessor
{
	constexpr explicit FileProcessor(const RunConfig &config) noexcept
	    : m_config(config)
	{
		static_assert(std::is_move_constructible_v<Context>);
		static_assert(std::is_move_constructible_v<RunConfig>);
		static_assert(std::is_move_constructible_v<GitRepository>);
	}

	void process();
	void process(const QString &targetPath);

private:
	const RunConfig &m_config;
};
