#pragma once
#include <string>

namespace litmus
{
	inline namespace internal
	{
#if defined(LITMUS_USE_COLOURS)
		inline std::string to_colour(uint8_t value) noexcept
		{
			constexpr static std::string_view str_values[10]{"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
			auto mod_100 = value % 100;
			auto mod_10	 = mod_100 % 10;
			std::string res{};
			if(value > 99)
			{
				res.append(str_values[(value - mod_100) / 100]);
			}
			if(mod_100 > 9)
			{
				res.append(str_values[(mod_100 - mod_10) / 10]);
			}
			res.append(str_values[mod_10]);
			return res;
		}
		inline auto bold(std::string str) -> std::string { return "\x1b[1m" + str + "\x1b[22m"; }
		inline auto dim(std::string str) -> std::string { return "\x1b[2m" + str + "\x1b[22m"; }
		inline auto italics(std::string str) -> std::string { return "\x1b[3m" + str + "\x1b[23m"; }
		inline auto underline(std::string str) -> std::string { return "\x1b[4m" + str + "\x1b[24m"; }
		inline auto double_underline(std::string str) -> std::string { return "\x1b[4:2m" + str + "\x1b[4:0m"; }
		inline auto curly(std::string str) -> std::string { return "\x1b[4:3m" + str + "\x1b[4:0m"; }
		inline auto strikethrough(std::string str) -> std::string { return "\x1b[9m" + str + "\x1b[29m"; }
		inline auto overline(std::string str) -> std::string { return "\x1b[53m" + str + "\x1b[55m"; }


		inline auto colour(std::string str, uint8_t red, uint8_t green, uint8_t blue) -> std::string
		{
			return "\x1b[38;2;" + to_colour(red) + ";" + to_colour(green) + ";" + to_colour(blue) + "m" + str +
				   "\x1b[39m";
		}

		inline auto colour(std::string str, const std::array<uint8_t, 3>& values) -> std::string
		{
			return "\x1b[38;2;" + to_colour(values[0]) + ";" + to_colour(values[1]) + ";" + to_colour(values[2]) + "m" +
				   str + "\x1b[39m";
		}
#else

		inline std::string to_colour(uint8_t value) noexcept
		{
			constexpr static std::string_view str_values[10]{"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
			auto mod_100 = value % 100;
			auto mod_10	 = mod_100 % 10;
			std::string res{};
			if(value > 99)
			{
				res.append(str_values[(value - mod_100) / 100]);
			}
			if(mod_100 > 9)
			{
				res.append(str_values[(mod_100 - mod_10) / 10]);
			}
			res.append(str_values[mod_10]);
			return res;
		}
		inline auto bold(std::string str) -> std::string { return str; }
		inline auto dim(std::string str) -> std::string { return str; }
		inline auto italics(std::string str) -> std::string { return str; }
		inline auto underline(std::string str) -> std::string { return str; }
		inline auto double_underline(std::string str) -> std::string { return str; }
		inline auto curly(std::string str) -> std::string { return str; }
		inline auto strikethrough(std::string str) -> std::string { return str; }
		inline auto overline(std::string str) -> std::string { return str; }


		inline auto colour(std::string str, uint8_t red, uint8_t green, uint8_t blue) -> std::string { return str; }

		inline auto colour(std::string str, const std::array<uint8_t, 3>& values) -> std::string { return str; }
#endif
	} // namespace internal
} // namespace litmus
