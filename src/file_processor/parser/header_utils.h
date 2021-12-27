#pragma once

#include <algorithm>
#include <array>
#include <optional>
#include <string_view>

namespace header_utils {

template <typename Key, typename Value, std::size_t Size>
struct Map
{
	std::array<std::pair<Key, Value>, Size> data;

	[[nodiscard]] constexpr std::size_t size() const noexcept { return data.size(); }

	[[nodiscard]] constexpr std::optional<Value> at(const Key &key) const noexcept
	{
		const auto itr =
		    std::find_if(begin(data), end(data), [&key](const auto &v) { return v.first == key; });
		return itr != end(data) ? itr->second : std::optional<Value>();
	}

	[[nodiscard]] constexpr Value operator[](const Key &key) const { return at(key).value(); }

	[[nodiscard]] constexpr auto cbegin() const noexcept { return data.cbegin(); }
	[[nodiscard]] constexpr auto cend() const noexcept { return data.cend(); }
};

namespace impl {

// clang-format off
constexpr auto cStarPrefix = "/******************************************************************************\n**\n";
constexpr auto cStarSuffix = "\n**\n******************************************************************************/\n\n";
constexpr auto cStarStart = "**";
constexpr auto cNumPrefix = "";
constexpr auto cNumSuffix = "\n\n";
constexpr auto cNumStart = "#";
// clang-format on

constexpr std::array<std::pair<std::string_view, std::string_view>, 12> prefix{{
    {"c", cStarPrefix},
    {"cmake", cNumPrefix},
    {"cpp", cStarPrefix},
    {"cxx", cStarPrefix},
    {"h", cStarPrefix},
    {"hpp", cStarPrefix},
    {"hxx", cStarPrefix},
    {"js", cStarPrefix},
    {"m", cStarPrefix},
    {"mm", cStarPrefix},
    //{ "py", cNumPrefix},
    {"qml", cStarPrefix},
    //"sh", cNumPrefix,
    {"swift", cStarPrefix},
    //{"ts", cStarPrefix},
}};

constexpr std::array<std::pair<std::string_view, std::string_view>, 12> suffix{{
    {"c", cStarSuffix},
    {"cmake", cNumSuffix},
    {"cpp", cStarSuffix},
    {"cxx", cStarSuffix},
    {"h", cStarSuffix},
    {"hpp", cStarSuffix},
    {"hxx", cStarSuffix},
    {"js", cStarSuffix},
    {"m", cStarSuffix},
    {"mm", cStarSuffix},
    //{ "py", cNumSuffix},
    {"qml", cStarSuffix},
    //"sh", cNumPrefix,
    {"swift", cStarSuffix},
    //{"ts", cNumSuffix},
}};

constexpr std::array<std::pair<std::string_view, std::string_view>, 12> start{{
    {"c", cStarStart},
    {"cmake", cNumStart},
    {"cpp", cStarStart},
    {"cxx", cStarStart},
    {"h", cStarStart},
    {"hpp", cStarStart},
    {"hxx", cStarStart},
    {"js", cStarStart},
    {"m", cStarStart},
    {"mm", cStarStart},
    //{ "py", cNumStart},
    {"qml", cStarStart},
    //"sh", cNumStart,
    {"swift", cStarStart},
    //{"ts", cStarPrefix},
}};

[[maybe_unused]] constexpr void _()
{
	static_assert(std::is_sorted(prefix.begin(), prefix.end()));
	static_assert(std::is_sorted(suffix.begin(), suffix.end()));
	static_assert(std::is_sorted(start.begin(), start.end()));
	static_assert(prefix.size() == suffix.size());
	static_assert(prefix.size() == start.size());
}

}  // namespace impl

constexpr auto prefixMap =
    Map<std::string_view, std::string_view, impl::prefix.size()>{{impl::prefix}};
constexpr auto suffixMap =
    Map<std::string_view, std::string_view, impl::suffix.size()>{{impl::suffix}};
constexpr auto startMap =
    Map<std::string_view, std::string_view, impl::start.size()>{{impl::start}};

}  // namespace header_utils