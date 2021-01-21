#pragma once
#include <cstdint>
#include <string_view>

namespace litmus
{
	inline namespace internal
	{
		template <size_t N>
		struct fixed_string;

		template <typename T>
		struct is_fixed_string : std::false_type
		{};
		template <size_t N>
		struct is_fixed_string<fixed_string<N>> : std::true_type
		{};

		template <typename T>
		concept IsFixedAsciiString = is_fixed_string<std::remove_cvref_t<T>>::value;

		template <size_t N>
		struct fixed_string
		{
			char buf[N + 1]{};
			consteval fixed_string(char const *s) noexcept
			{
				for(size_t i = 0; i != N; ++i) buf[i] = s[i];
			}
			consteval fixed_string(const char (&s)[N]) noexcept
			{
				for(size_t i = 0; i != N; ++i) buf[i] = s[i];
			}
			consteval fixed_string(const fixed_string &s) noexcept
			{
				for(size_t i = 0; i != N; ++i) buf[i] = s[i];
			}

			auto operator<=>(const fixed_string &) const = default;

			constexpr char operator[](size_t index) const noexcept { return buf[index]; }

			constexpr operator std::string_view() const noexcept { return std::string_view{buf, N}; }
			constexpr operator char const *() const { return buf; }

			constexpr size_t size() const noexcept { return N; }

			template <size_t start, size_t end>
			consteval fixed_string<end - start> substr() const noexcept
			{
				static_assert(start <= end);
				static_assert(end <= N + 1);
				return fixed_string<end - start>{&buf[start]};
			}
		};
		template <unsigned N>
		fixed_string(char const (&)[N]) -> fixed_string<N - 1>;


		template <size_t N>
		fixed_string(const fixed_string<N> &) -> fixed_string<N>;

	} // namespace internal
} // namespace litmus