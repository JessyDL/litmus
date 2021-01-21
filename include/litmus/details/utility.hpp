#pragma once

#include <string>

namespace litmus
{
	inline namespace internal
	{

		inline auto join(const std::vector<std::string>& str, std::string_view character) -> std::string
		{
			if(str.size() == 0) return "";

			const auto size = std::accumulate(std::begin(str), std::end(str), size_t{0},
											  [](size_t sum, const auto& str) { return sum + str.size(); }) +
							  ((str.size() - 1) * character.size());
			std::string res{};
			res.reserve(size);

			for(auto i = 0u; i < str.size() - 1u; ++i)
			{
				res += str[i];
				res += character;
			}
			res += str.back();
			return res;
		}

		template <typename T>
		inline auto text_size(const T& value) noexcept -> size_t
		{
			return value.size();
		}

		template <size_t N>
		inline auto text_size(const char (&)[N]) noexcept -> size_t
		{
			return N - 1;
		}

		inline auto text_size(const char&) noexcept -> size_t { return {1}; }

		template <typename... Ts>
		inline auto combine_text(Ts&&... values) -> std::string
		{
			const auto size = (text_size(values) + ...);
			std::string res{};
			res.reserve(size);
			(void(res += std::forward<Ts>(values)), ...);
			return res;
		}
	} // namespace internal

} // namespace litmus