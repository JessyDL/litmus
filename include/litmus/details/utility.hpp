#pragma once

#include <string>
#include <type_traits>

#include "strtype/strtype.hpp"

namespace litmus
{
	template <typename T>
	inline auto type_to_name(std::type_identity<T>) -> std::string
	{
		return std::string { strtype::stringify_typename<T>() };
	}

	template <>
	inline auto type_to_name(std::type_identity<uint8_t>) -> std::string
	{
		return "ui8";
	}
	template <>
	inline auto type_to_name(std::type_identity<uint16_t>) -> std::string
	{
		return "ui16";
	}
	template <>
	inline auto type_to_name(std::type_identity<uint32_t>) -> std::string
	{
		return "ui32";
	}
	template <>
	inline auto type_to_name(std::type_identity<uint64_t>) -> std::string
	{
		return "ui64";
	}
	template <>
	inline auto type_to_name(std::type_identity<int8_t>) -> std::string
	{
		return "i8";
	}
	template <>
	inline auto type_to_name(std::type_identity<int16_t>) -> std::string
	{
		return "i16";
	}
	template <>
	inline auto type_to_name(std::type_identity<int32_t>) -> std::string
	{
		return "i32";
	}
	template <>
	inline auto type_to_name(std::type_identity<int64_t>) -> std::string
	{
		return "i64";
	}
	template <>
	inline auto type_to_name(std::type_identity<float>) -> std::string
	{
		return "float";
	}
	template <>
	inline auto type_to_name(std::type_identity<double>) -> std::string
	{
		return "double";
	}
	template <>
	inline auto type_to_name(std::type_identity<bool>) -> std::string
	{
		return "bool";
	}

	inline namespace internal
	{
		template <typename T>
		inline auto type_to_name_internal() -> std::string
		{
			return type_to_name(std::type_identity<std::remove_cvref_t<T>>{});
		}

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
		inline auto text_size([[maybe_unused]] const char (&str)[N]) noexcept -> size_t
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
