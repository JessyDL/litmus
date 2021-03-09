#pragma once
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>

#include <litmus/details/context.hpp>
#include <litmus/details/source_location.hpp>
#include <litmus/details/test_result.hpp>
#include <litmus/details/utility.hpp>

namespace litmus
{
	inline namespace internal
	{
		template <typename... Ts>
		struct any_of_t
		{
			std::tuple<Ts...> values;
		};

		template <typename... Ts>
		struct throws_t
		{};

		struct nothrows_t
		{};

		template <typename T>
		struct is_throwable : std::false_type
		{};

		template <typename... Ts>
		struct is_throwable<throws_t<Ts...>> : std::true_type
		{};

		template <>
		struct is_throwable<nothrows_t> : std::true_type
		{};

		template <typename T>
		concept IsThrowable = is_throwable<std::remove_cvref_t<T>>::value;

		struct throws_result_t
		{
			constexpr throws_result_t() noexcept = default;
			throws_result_t(bool value) noexcept : result(value) {}
			throws_result_t(bool value, std::string what) noexcept : result(value), what(what) {}

			template <typename... Ts>
			constexpr bool operator==(const throws_t<Ts...>&) const noexcept
			{
				return result;
			}
			template <typename... Ts>
			constexpr bool operator!=(const throws_t<Ts...>&) const noexcept
			{
				return !result;
			}
			constexpr bool operator==(const nothrows_t&) const noexcept { return !result; }
			constexpr bool operator!=(const nothrows_t&) const noexcept { return result; }

			operator bool() const noexcept { return result; }
			bool result{false};
			std::string what{};
		};

		template <typename T>
		concept IsStringType = requires(T t)
		{
			static_cast<std::string>(t);
		};
	} // namespace internal

	template <typename T>
	struct value_to_string_t
	{
		constexpr std::string operator()(const T& value) const noexcept
		{
			using std::to_string;
			if constexpr(IsStringType<T>)
				return value;
			else
				return to_string(value);
		}
	};


	auto value_to_string(const auto& value) -> std::string
	{
		return value_to_string_t<std::remove_cvref_t<decltype(value)>>{}(value);
	}

	template <typename... Ts>
	struct value_to_string_t<throws_t<Ts...>>
	{
		using type = throws_t<Ts...>;
		constexpr std::string operator()([[maybe_unused]] const type& value) const noexcept
		{
			std::string res{"throws<"};
			if constexpr(sizeof...(Ts) > 0)
			{
				res += ((type_to_name(std::type_identity<Ts>{}) + ", ") + ...);
				res.erase(res.size() - 1);
				res.back() = '>';
			}
			else
			{
				res += "std::exception>";
			}
			return res;
		}
	};

	inline namespace internal
	{
		template <typename T>
		concept IsStringifyable = requires(const T& val)
		{
			{
				value_to_string(val)
			}
			->std::same_as<std::string>;
		};
		auto to_string_fn(const auto& val) noexcept -> std::string
		{
			using T = std::remove_cvref_t<typename std::decay<decltype(val)>::type>;
			if constexpr(std::is_same_v<T, bool>)
			{
				return (val) ? "true" : "false";
			}
			else if constexpr(std::is_same_v<T, throws_result_t>)
			{
				return ((val == true) ? std::string("EXCEPT") : std::string("NOEXCEPT")) +
					   ((val.what.empty()) ? "" : ": ") + val.what;
			}
			else if constexpr(IsStringifyable<T>)
			{
				std::string res = value_to_string(val);

				if constexpr(std::is_integral_v<T> && std::is_unsigned_v<T>)
				{
					return res + "u";
				}
				return res;
			}
			else
			{
				static_assert(!std::is_same_v<T, T>, "type cannot be converted to a string");
			}
			return "";
		}

		template <typename Ex, typename... ExRemainder>
		constexpr auto throws_fn_impl(auto& fn, auto&... args) -> throws_result_t
		{
			try
			{
				if constexpr(sizeof...(ExRemainder) == 0)
				{
					fn(args...);
				}
				else
				{
					return throws_fn_impl<ExRemainder...>(fn, args...);
				}
			}
			catch(const Ex& e)
			{
				return {true, e.what()};
			}
			return {false};
		}

		template <typename... Exceptions>
		constexpr auto throws_fn(auto& fn, auto&... args) noexcept -> throws_result_t
		{
			try
			{
				if constexpr(sizeof...(Exceptions) == 0)
				{
					fn(args...);
				}
				else
				{
					return throws_fn_impl<Exceptions...>(fn, args...);
				}
			}
			catch(...)
			{
				return {true, "unknown exception raised."};
			}
			return {false};
		}


		void evaluate(const source_location& source, test_result_t::expect_t::operation_t operation,
					  std::string_view keyword, std::string& lhs_user, std::string& rhs_user);

		static thread_local struct
		{
			std::string message;
		} expect_info;

		template <bool Fatal>
		constexpr inline void log_expect(const auto& lhs, const auto& rhs, bool res,
										 test_result_t::expect_t::operation_t operation,
										 const source_location& location) noexcept
		{
			std::string lhs_user{};
			std::string rhs_user{};
			evaluate(location, operation, (Fatal) ? "require" : "expect", lhs_user, rhs_user);
			suite_context.output.expect_result(to_string_fn(lhs), to_string_fn(rhs), lhs_user, rhs_user, operation, res,
											   Fatal, expect_info.message);
			expect_info.message = {};

			suite_context.output.fatal = !res && Fatal;
		}

		template <bool Fatal, typename T>
		class expect_t
		{
		  public:
			template <typename Y>
			constexpr expect_t(const source_location& source, Y&& value) noexcept
				: m_Value(std::forward<Y>(value)), m_Source(source)
			{}
			auto operator==(const auto& rhs) const noexcept -> bool
			{
				if(suite_context.output.fatal) return false;
				const bool res{m_Value == rhs};
				log_expect<Fatal>(m_Value, rhs, res, test_result_t::expect_t::operation_t::equal, m_Source);
				return res;
			}
			auto operator!=(const auto& rhs) const noexcept -> bool
			{
				if(suite_context.output.fatal) return false;
				const bool res{m_Value != rhs};
				log_expect<Fatal>(m_Value, rhs, res, test_result_t::expect_t::operation_t::inequal, m_Source);
				return res;
			}
			auto operator<(const auto& rhs) const noexcept -> bool
			{
				if(suite_context.output.fatal) return false;
				const bool res{m_Value < rhs};
				log_expect<Fatal>(m_Value, rhs, res, test_result_t::expect_t::operation_t::less_than, m_Source);
				return res;
			}
			auto operator>(const auto& rhs) const noexcept -> bool
			{
				if(suite_context.output.fatal) return false;
				const bool res{m_Value > rhs};
				log_expect<Fatal>(m_Value, rhs, res, test_result_t::expect_t::operation_t::greater_than, m_Source);
				return res;
			}
			auto operator<=(const auto& rhs) const noexcept -> bool
			{
				if(suite_context.output.fatal) return false;
				const bool res{m_Value <= rhs};
				log_expect<Fatal>(m_Value, rhs, res, test_result_t::expect_t::operation_t::less_equal, m_Source);
				return res;
			}
			auto operator>=(const auto& rhs) const noexcept -> bool
			{
				if(suite_context.output.fatal) return false;
				const bool res{m_Value >= rhs};
				log_expect<Fatal>(m_Value, rhs, res, test_result_t::expect_t::operation_t::greater_equal, m_Source);
				return res;
			}


		  private:
			T m_Value;
			source_location m_Source;
		};

		template <bool Fatal, typename Fn, typename... Args>
		class expect_invocable_t
		{
		  public:
			template <typename FnFw, typename... ArgsFw>
			constexpr expect_invocable_t(const source_location& source, FnFw&& fun, ArgsFw&&... args)
				: m_Fun(fun), m_Args(std::forward<ArgsFw>(args)...), m_Source(source)
			{}

			auto operator==(const nothrows_t& rhs) const noexcept -> bool
			{
				if(suite_context.output.fatal) return false;
				const auto value =
					std::apply([& fun = m_Fun](auto&... args) { return throws_fn<>(fun, args...); }, m_Args);
				const bool res{value == rhs};
				log_expect<Fatal>(value, "nothrows", res, test_result_t::expect_t::operation_t::equal, m_Source);
				return res;
			}
			auto operator!=(const nothrows_t& rhs) const noexcept -> bool
			{
				if(suite_context.output.fatal) return false;
				const auto value =
					std::apply([& fun = m_Fun](auto&... args) { return throws_fn<>(fun, args...); }, m_Args);
				const bool res{value != rhs};
				log_expect<Fatal>(value, "nothrows", res, test_result_t::expect_t::operation_t::equal, m_Source);
				return res;
			}
			template <typename... Exceptions>
			auto operator==(const throws_t<Exceptions...>& rhs) const noexcept -> bool
			{
				if(suite_context.output.fatal) return false;
				const auto value = std::apply(
					[& fun = m_Fun](auto&... args) { return throws_fn<Exceptions...>(fun, args...); }, m_Args);
				const bool res{value == rhs};
				log_expect<Fatal>(value, rhs, res, test_result_t::expect_t::operation_t::equal, m_Source);
				return res;
			}

			template <typename... Exceptions>
			auto operator!=(const throws_t<Exceptions...>& rhs) const noexcept -> bool
			{
				if(suite_context.output.fatal) return false;
				const auto value = std::apply(
					[& fun = m_Fun](auto&... args) { return throws_fn<Exceptions...>(fun, args...); }, m_Args);
				const bool res{value != rhs};
				log_expect<Fatal>(value, rhs, res, test_result_t::expect_t::operation_t::equal, m_Source);
				return res;
			}

			auto operator==(const auto& rhs) const noexcept -> bool requires(!IsThrowable<decltype(rhs)>)
			{
				if(suite_context.output.fatal) return false;
				const auto value = std::apply(m_Fun, m_Args);
				const bool res{value == rhs};
				log_expect<Fatal>(value, rhs, res, test_result_t::expect_t::operation_t::equal, m_Source);
				return res;
			}
			auto operator!=(const auto& rhs) const noexcept -> bool requires(!IsThrowable<decltype(rhs)>)
			{
				if(suite_context.output.fatal) return false;
				const auto value = std::apply(m_Fun, m_Args);
				const bool res{value != rhs};
				log_expect<Fatal>(value, rhs, res, test_result_t::expect_t::operation_t::inequal, m_Source);
				return res;
			}

			auto operator<(const auto& rhs) const noexcept -> bool
			{
				if(suite_context.output.fatal) return false;
				const auto value = std::apply(m_Fun, m_Args);
				const bool res{value < rhs};
				log_expect<Fatal>(value, rhs, res, test_result_t::expect_t::operation_t::less_than, m_Source);
				return res;
			}
			auto operator>(const auto& rhs) const noexcept -> bool
			{
				if(suite_context.output.fatal) return false;
				const auto value = std::apply(m_Fun, m_Args);
				const bool res{value > rhs};
				log_expect<Fatal>(value, rhs, res, test_result_t::expect_t::operation_t::greater_than, m_Source);
				return res;
			}
			auto operator<=(const auto& rhs) const noexcept -> bool
			{
				if(suite_context.output.fatal) return false;
				const auto value = std::apply(m_Fun, m_Args);
				const bool res{value <= rhs};
				log_expect<Fatal>(value, rhs, res, test_result_t::expect_t::operation_t::less_equal, m_Source);
				return res;
			}
			auto operator>=(const auto& rhs) const noexcept -> bool
			{
				if(suite_context.output.fatal) return false;
				const auto value = std::apply(m_Fun, m_Args);
				const bool res{value >= rhs};
				log_expect<Fatal>(value, rhs, res, test_result_t::expect_t::operation_t::greater_equal, m_Source);
				return res;
			}

		  private:
			Fn m_Fun;
			std::tuple<Args...> m_Args;
			source_location m_Source;
		};

		template <typename T, typename Y, typename... Ts>
		concept IsInvocableMemFn = requires(T t, Y y)
		{
			std::mem_fn(t, y);
		};

	} // namespace internal

	template <typename T, typename... Ts>
	struct expect : public std::conditional_t<std::is_invocable_v<T, Ts...>, expect_invocable_t<false, T, Ts...>,
											  expect_t<false, T>>
	{
		using base =
			std::conditional_t<std::is_invocable_v<T, Ts...>, expect_invocable_t<false, T, Ts...>, expect_t<false, T>>;
		expect(T&& t, Ts&&... ts, const source_location& loc = source_location::current())
			: base(loc, std::forward<T>(t), std::forward<Ts>(ts)...)
		{}
	};

	template <typename T, typename... Ts>
	expect(T&&, Ts&&...)->expect<T, Ts...>;

	template <typename T, typename... Ts>
	struct expect_true : public expect<T, Ts...>
	{
		using base = expect<T, Ts...>;
		expect_true(T&& t, Ts&&... ts, const source_location& loc = source_location::current())
			: base(std::forward<T>(t), std::forward<Ts>(ts)..., loc), m_Result(base::operator==(true))
		{}

		constexpr operator bool() const noexcept { return m_Result; }

	  private:
		const bool m_Result{};
	};

	template <typename T, typename... Ts>
	expect_true(T&&, Ts&&...)->expect_true<T, Ts...>;

	template <typename T, typename... Ts>
	struct expect_false : public expect<T, Ts...>
	{
		using base = expect<T, Ts...>;
		expect_false(T&& t, Ts&&... ts, const source_location& loc = source_location::current())
			: base(std::forward<T>(t), std::forward<Ts>(ts)..., loc), m_Result(base::operator==(false))
		{}

		constexpr operator bool() const noexcept { return m_Result; }

	  private:
		const bool m_Result{};
	};

	template <typename T, typename... Ts>
	expect_false(T&&, Ts&&...)->expect_false<T, Ts...>;

	template <typename T, typename... Ts>
	struct require : public std::conditional_t<std::is_invocable_v<T, Ts...>, expect_invocable_t<true, T, Ts...>,
											   expect_t<true, T>>
	{
		using base =
			std::conditional_t<std::is_invocable_v<T, Ts...>, expect_invocable_t<true, T, Ts...>, expect_t<true, T>>;
		require(T&& t, Ts&&... ts, const source_location& loc = source_location::current())
			: base(loc, std::forward<T>(t), std::forward<Ts>(ts)...)
		{}
	};

	template <typename T, typename... Ts>
	require(T&&, Ts&&...)->require<T, Ts...>;

	template <typename T, typename... Ts>
	struct require_true : public require<T, Ts...>
	{
		using base = require<T, Ts...>;
		require_true(T&& t, Ts&&... ts, const source_location& loc = source_location::current())
			: base(std::forward<T>(t), std::forward<Ts>(ts)..., loc), m_Result(base::operator==(true))
		{}

		constexpr operator bool() const noexcept { return m_Result; }

	  private:
		const bool m_Result{};
	};

	template <typename T, typename... Ts>
	require_true(T&&, Ts&&...)->require_true<T, Ts...>;

	template <typename T, typename... Ts>
	struct require_false : public require<T, Ts...>
	{
		using base = require<T, Ts...>;
		require_false(T&& t, Ts&&... ts, const source_location& loc = source_location::current())
			: base(std::forward<T>(t), std::forward<Ts>(ts)..., loc), m_Result(base::operator==(false))
		{}

		constexpr operator bool() const noexcept { return m_Result; }

	  private:
		const bool m_Result{};
	};

	template <typename T, typename... Ts>
	require_false(T&&, Ts&&...)->require_false<T, Ts...>;


	template <IsStringifyable... Ts>
	void info(Ts&&... values)
	{
		if(suite_context.output.fatal) return;
		expect_info.message = std::move(combine_text(value_to_string(std::forward<Ts>(values))...));
	}

	template <typename... Ts>
	[[nodiscard]] constexpr auto throws() noexcept
		-> std::conditional_t<sizeof...(Ts) == 0, throws_t<std::exception>, throws_t<Ts...>>
	{
		return {};
	}

	[[nodiscard]] constexpr auto nothrows() noexcept -> nothrows_t { return {}; }


} // namespace litmus
