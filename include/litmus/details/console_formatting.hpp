#pragma once
#include <string>

namespace litmus
{
	inline namespace internal
	{
		inline auto bold(std::string str) -> std::string { return "\x1b[1m" + str + "\x1b[22m"; }
		inline auto dim(std::string str) -> std::string { return "\x1b[2m" + str + "\x1b[22m"; }
		inline auto italics(std::string str) -> std::string { return "\x1b[3m" + str + "\x1b[23m"; }
		inline auto underline(std::string str) -> std::string { return "\x1b[4m" + str + "\x1b[24m"; }
		inline auto double_underline(std::string str) -> std::string { return "\x1b[4:2m" + str + "\x1b[4:0m"; }
		inline auto curly(std::string str) -> std::string { return "\x1b[4:3m" + str + "\x1b[4:0m"; }
		inline auto strikethrough(std::string str) -> std::string { return "\x1b[9m" + str + "\x1b[29m"; }
		inline auto overline(std::string str) -> std::string { return "\x1b[53m" + str + "\x1b[55m"; }

		inline auto colour(std::string str, const std::string& red, const std::string& green, const std::string& blue)
			-> std::string
		{
			return "\x1b[38;2;" + red + ";" + green + ";" + blue + "m" + str + "\x1b[39m";
		}
	} // namespace internal
} // namespace litmus