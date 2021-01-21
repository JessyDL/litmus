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
		// template<typename...Ts>
		// struct any_of_t
		// {
		// 	std::tuple<Ts...> values;
		// };

		struct throws_result_t
		{
			constexpr throws_result_t() noexcept = default;
			throws_result_t(bool value) noexcept : result(value) {}
			throws_result_t(bool value, std::string what) noexcept : result(value), what(what) {}
			operator bool() const noexcept { return result; }
			bool result{false};
			std::string what{};
		};

		struct nothrows_result_t
		{
			constexpr nothrows_result_t() noexcept = default;
			nothrows_result_t(bool value) noexcept : result(value) {}
			nothrows_result_t(bool value, std::string what) noexcept : result(value), what(what) {}
			operator bool() const noexcept { return result; }
			bool result{false};
			std::string what{};
		};
	} // namespace internal

	template <typename... Exceptions>
	struct throws_t
	{
	  private:
		template <typename E, typename... Ex, typename... Ts, typename Rt, typename T>
		auto throws_impl_mfn(Rt (T::*address)(Ts...), T& object, Ts&&... args) noexcept -> throws_result_t
		{
			try
			{
				if constexpr(sizeof...(Ex) == 0)
				{
					std::invoke(address, &object, std::forward<Ts>(args)...);
					return false;
				}
				else
					return throws_impl_mfn<Ex...>(address, object, std::forward<Ts>(args)...);
			}
			catch(const E& e)
			{
				return {true, e.what()};
			}
			return false;
		}

		template <typename E, typename... Ex, typename... Ts, typename Rt, typename T>
		auto throws_impl_cmfn(Rt (T::*address)(Ts...) const, T& object, Ts&&... args) const noexcept -> throws_result_t
		{
			try
			{
				if constexpr(sizeof...(Ex) == 0)
				{
					std::invoke(address, &object, std::forward<Ts>(args)...);
					return false;
				}
				else
					return throws_impl_cmfn<Ex...>(address, object, std::forward<Ts>(args)...);
			}
			catch(const E& e)
			{
				return {true, e.what()};
			}
			return false;
		}

		template <typename E, typename... Ex, typename... Ts, typename Rt>
		auto throws_impl_fn(Rt (*address)(Ts...), Ts&&... args) const noexcept -> throws_result_t
		{
			try
			{
				if constexpr(sizeof...(Ex) == 0)
				{
					std::invoke(address, std::forward<Ts>(args)...);
					return false;
				}
				else
					return throws_impl_fn<Ex...>(address, std::forward<Ts>(args)...);
			}
			catch(const E& e)
			{
				return {true, e.what()};
			}
			return false;
		}

	  public:
		template <typename... Ts, typename T, typename Rt = void>
		auto operator()(Rt (T::*address)(Ts...), T& object, Ts&&... args) noexcept -> throws_result_t
		{
			try
			{
				if constexpr(sizeof...(Exceptions) == 0)
				{
					std::invoke(address, &object, std::forward<Ts>(args)...);
					return false;
				}
				else
					return throws_impl_mfn<Exceptions...>(address, object, std::forward<Ts>(args)...);
			}
			catch(const std::exception& e)
			{
				return {sizeof...(Exceptions) == 0, e.what()};
			}
			catch(...)
			{
				return sizeof...(Exceptions) == 0;
			}
			return false;
		}

		template <typename... Ts, typename T, typename Rt = void>
		auto operator()(Rt (T::*address)(Ts...) const, T& object, Ts&&... args) const noexcept -> throws_result_t
		{
			try
			{
				if constexpr(sizeof...(Exceptions) == 0)
				{
					std::invoke(address, &object, std::forward<Ts>(args)...);
					return false;
				}
				else
					return throws_impl_cmfn<Exceptions...>(address, object, std::forward<Ts>(args)...);
			}
			catch(const std::exception& e)
			{
				return {sizeof...(Exceptions) == 0, e.what()};
			}
			catch(...)
			{
				return sizeof...(Exceptions) == 0;
			}
			return false;
		}

		template <typename... Ts, typename Rt = void>
		auto operator()(Rt (*address)(Ts...), Ts&&... args) const noexcept -> throws_result_t
		{
			try
			{
				if constexpr(sizeof...(Exceptions) == 0)
				{
					std::invoke(address, std::forward<Ts>(args)...);
					return false;
				}
				else
					return throws_impl_fn<Exceptions...>(address, std::forward<Ts>(args)...);
			}
			catch(const std::exception& e)
			{
				return {sizeof...(Exceptions) == 0, e.what()};
			}
			catch(...)
			{
				return sizeof...(Exceptions) == 0;
			}
			return false;
		}

		template <typename... Ts, typename T, typename Rt = void>
		auto invoke(Rt (T::*address)(Ts...), T& object, Ts&&... args) noexcept
		{
			return this->template operator()<Ts...>(address, object, std::forward<Ts>(args)...);
		}

		template <typename... Ts, typename T, typename Rt = void>
		auto invoke(Rt (T::*address)(Ts...) const, T& object, Ts&&... args) const noexcept
		{
			return this->template operator()<Ts...>(address, object, std::forward<Ts>(args)...);
		}

		template <typename... Ts, typename Rt = void>
		auto invoke(Rt (*address)(Ts...), Ts&&... args) const noexcept
		{
			return this->template operator()<Ts...>(address, std::forward<Ts>(args)...);
		}
	};

	struct nothrows_t
	{
		template <typename... Ts, typename T, typename Rt = void>
		auto operator()(Rt (T::*address)(Ts...), T& object, Ts&&... args) noexcept -> nothrows_result_t
		{
			try
			{
				std::invoke(address, &object, std::forward<Ts>(args)...);
			}
			catch(const std::exception& e)
			{
				return {false, e.what()};
			}
			catch(...)
			{
				return false;
			}
			return true;
		}

		template <typename... Ts, typename T, typename Rt = void>
		auto operator()(Rt (T::*address)(Ts...) const, T& object, Ts&&... args) const noexcept -> nothrows_result_t
		{

			try
			{
				std::invoke(address, &object, std::forward<Ts>(args)...);
			}
			catch(const std::exception& e)
			{
				return {false, e.what()};
			}
			catch(...)
			{
				return false;
			}
			return true;
		}

		template <typename... Ts, typename Rt = void>
		auto operator()(Rt (*address)(Ts...), Ts&&... args) const noexcept -> nothrows_result_t
		{
			try
			{
				std::invoke(address, std::forward<Ts>(args)...);
			}
			catch(const std::exception& e)
			{
				return {false, e.what()};
			}
			catch(...)
			{
				return false;
			}
			return true;
		}

		template <typename... Ts, typename T, typename Rt = void>
		auto invoke(Rt (T::*address)(Ts...), T& object, Ts&&... args) noexcept
		{
			return this->template operator()<Ts...>(address, object, std::forward<Ts>(args)...);
		}

		template <typename... Ts, typename T, typename Rt = void>
		auto invoke(Rt (T::*address)(Ts...) const, T& object, Ts&&... args) const noexcept
		{
			return this->template operator()<Ts...>(address, object, std::forward<Ts>(args)...);
		}

		template <typename... Ts, typename Rt = void>
		auto invoke(Rt (*address)(Ts...), Ts&&... args) const noexcept
		{
			return this->template operator()<Ts...>(address, std::forward<Ts>(args)...);
		}
	};

	inline namespace internal
	{
		template <typename T>
		concept IsStringifyable = requires(const T& val)
		{
			{
				std::to_string(val)
			}
			->std::same_as<std::string>;
		};
		auto to_string(const auto& val) noexcept -> std::string
		{
			using T = typename std::decay<decltype(val)>::type;
			if constexpr(std::is_same_v<T, bool>)
			{
				return (val) ? "true" : "false";
			}

			if constexpr(std::is_same_v<T, throws_result_t>)
			{
				return ((val == true) ? std::string("EXCEPT") : std::string("NOEXCEPT")) +
					   ((val.what.empty()) ? "" : ": ") + val.what;
			}
			if constexpr(std::is_same_v<T, nothrows_result_t>)
			{
				return ((val == true) ? std::string("NOEXCEPT") : std::string("EXCEPT")) +
					   ((val.what.empty()) ? "" : ": ") + val.what;
			}
			std::string res;
			if constexpr(IsStringifyable<T>) res = std::to_string(val);

			if constexpr(std::is_integral_v<T> && std::is_unsigned_v<T>)
			{
				return res + "u";
			}
			return res;
		}
		auto to_string(const auto& val, auto& user_string) noexcept -> std::string
		{
			using T = typename std::decay<decltype(val)>::type;
			std::string res;
			if constexpr(IsStringifyable<T>) res = std::to_string(val);
			if constexpr(std::is_integral_v<T> && std::is_unsigned_v<T>)
			{
				if(user_string.size() == res.size() + 1 && user_string[user_string.size() - 1] == 'u')
					user_string.erase(user_string.size() - 1);
			}
			if constexpr(std::is_same_v<T, bool>)
			{
				res = (val) ? "true" : "false";
			}
			return res;
		}

		void evaluate(const source_location& source, test_result_t::expect_t::operation_t operation,
					  std::string_view keyword, std::string& lhs_user, std::string& rhs_user);

		static thread_local struct
		{
			std::string message;
		} expect_info;

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
				log(rhs, res, test_result_t::expect_t::operation_t::equal);
				return res;
			}
			auto operator!=(const auto& rhs) const noexcept -> bool
			{
				if(suite_context.output.fatal) return false;
				const bool res{m_Value != rhs};
				log(rhs, res, test_result_t::expect_t::operation_t::inequal);
				return res;
			}
			auto operator<(const auto& rhs) const noexcept -> bool
			{
				if(suite_context.output.fatal) return false;
				const bool res{m_Value < rhs};
				log(rhs, res, test_result_t::expect_t::operation_t::less_than);
				return res;
			}
			auto operator>(const auto& rhs) const noexcept -> bool
			{
				if(suite_context.output.fatal) return false;
				const bool res{m_Value > rhs};
				log(rhs, res, test_result_t::expect_t::operation_t::greater_than);
				return res;
			}
			auto operator<=(const auto& rhs) const noexcept -> bool
			{
				if(suite_context.output.fatal) return false;
				const bool res{m_Value <= rhs};
				log(rhs, res, test_result_t::expect_t::operation_t::less_equal);
				return res;
			}
			auto operator>=(const auto& rhs) const noexcept -> bool
			{
				if(suite_context.output.fatal) return false;
				const bool res{m_Value >= rhs};
				log(rhs, res, test_result_t::expect_t::operation_t::greater_equal);
				return res;
			}


		  private:
			void log(const auto& rhs, bool res, test_result_t::expect_t::operation_t operation) const noexcept
			{
				std::string lhs_user{};
				std::string rhs_user{};
				evaluate(m_Source, operation, (Fatal) ? "require" : "expect", lhs_user, rhs_user);
				suite_context.output.expect_result(to_string(m_Value), to_string(rhs), lhs_user, rhs_user, operation,
												   res, Fatal, expect_info.message);
				expect_info.message		   = {};
				suite_context.output.fatal = !res && Fatal;
			}
			T m_Value;
			const source_location& m_Source;
		};

		template <bool Fatal, typename Fn, typename... Args>
		class expect_invocable_t
		{
		  public:
			template <typename FnFw, typename... ArgsFw>
			constexpr expect_invocable_t(const source_location& source, FnFw&& fun, ArgsFw&&... args)
				: m_Fun(fun), m_Args(std::forward<ArgsFw>(args)...), m_Source(source)
			{}

			auto operator==(const auto& rhs) const noexcept -> bool
			{
				if(suite_context.output.fatal) return false;
				const auto value = std::apply(m_Fun, m_Args);
				const bool res{value == rhs};
				log(value, rhs, res, test_result_t::expect_t::operation_t::equal);
				return res;
			}
			auto operator!=(const auto& rhs) const noexcept -> bool
			{
				if(suite_context.output.fatal) return false;
				const auto value = std::apply(m_Fun, m_Args);
				const bool res{value != rhs};
				log(value, rhs, res, test_result_t::expect_t::operation_t::inequal);
				return res;
			}
			auto operator<(const auto& rhs) const noexcept -> bool
			{
				if(suite_context.output.fatal) return false;
				const auto value = std::apply(m_Fun, m_Args);
				const bool res{value < rhs};
				log(value, rhs, res, test_result_t::expect_t::operation_t::less_than);
				return res;
			}
			auto operator>(const auto& rhs) const noexcept -> bool
			{
				if(suite_context.output.fatal) return false;
				const auto value = std::apply(m_Fun, m_Args);
				const bool res{value > rhs};
				log(value, rhs, res, test_result_t::expect_t::operation_t::greater_than);
				return res;
			}
			auto operator<=(const auto& rhs) const noexcept -> bool
			{
				if(suite_context.output.fatal) return false;
				const auto value = std::apply(m_Fun, m_Args);
				const bool res{value <= rhs};
				log(value, rhs, res, test_result_t::expect_t::operation_t::less_equal);
				return res;
			}
			auto operator>=(const auto& rhs) const noexcept -> bool
			{
				if(suite_context.output.fatal) return false;
				const auto value = std::apply(m_Fun, m_Args);
				const bool res{value >= rhs};
				log(value, rhs, res, test_result_t::expect_t::operation_t::greater_equal);
				return res;
			}

		  private:
			void log(const auto& lhs, const auto& rhs, bool res, test_result_t::expect_t::operation_t operation) const
				noexcept
			{
				std::string lhs_user{};
				std::string rhs_user{};
				evaluate(m_Source, operation, (Fatal) ? "require" : "expect", lhs_user, rhs_user);
				suite_context.output.expect_result(to_string(lhs), to_string(rhs), lhs_user, rhs_user, operation, res,
												   Fatal, expect_info.message);
				expect_info.message = {};

				suite_context.output.fatal = !res && Fatal;
			}
			Fn m_Fun;
			std::tuple<Args...> m_Args;
			const source_location& m_Source;
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

	inline namespace internal
	{
		template <typename T>
		inline auto internal_to_string(const T& value) -> std::string
		{
			return std::to_string(value);
		}

		inline auto internal_to_string(const std::string& value) -> const std::string& { return value; }

		inline auto internal_to_string(const char* value) -> std::string_view { return value; }
	} // namespace internal


	template <typename... Ts>
	void info(Ts&&... values)
	{
		if(suite_context.output.fatal > 0) return;
		expect_info.message = std::move(combine_text(internal_to_string(std::forward<Ts>(values))...));
	}


} // namespace litmus