#pragma once
#include <iostream>
#include <stdexcept>
#include <tuple>
#include <litmus/details/fixed_string.hpp>
#include <litmus/details/scope.hpp>
#include <litmus/details/test_result.hpp>

namespace litmus
{
	inline namespace internal
	{
		struct section_functor
		{
			section_functor()
			{
				if(suite_context.index == std::numeric_limits<LITMUS_MAX_TEST_ID_TYPE>::max())
					throw std::runtime_error(
						"reached maximum indices available for tests, please increase 'LITMUS_MAX_TEST_ID_TYPE' to a "
						"larger type");
				m_Depth = suite_context.depth;
				m_Index = static_cast<LITMUS_MAX_TEST_ID_TYPE>(suite_context.index);
				suite_context.index += 1;
			}

			bool should_run()
			{
				if(suite_context.is_benchmark && m_Depth > 0)
					throw std::runtime_error("Cannot have nested sections in benchmarks");
				if(suite_context.output.fatal) return false;
				if(suite_context.bail)
				{
					if(suite_context.stack.empty())
					{
						suite_context.stack.set(m_Depth, m_Index);
					}
					return false;
				}

				return (suite_context.stack.empty() || suite_context.stack.get(m_Depth) == m_Index);
			}

			template <typename... InvokeTypes, typename... Ts>
			constexpr void operator()(auto& fn, const char* name, const source_location& location,
									  const std::vector<const char*>& categories, Ts&&... values)
			{
				if(!categories.empty()) throw std::runtime_error("not implemented");
				// we clear the stack when we detect it is the last "known" section we need to play.
				// this results in subsequent sections triggering "suite_context.stack.empty()".
				if(suite_context.stack.size() == m_Depth + 1 && suite_context.stack.get(m_Depth) == m_Index)
					suite_context.stack.clear();

				auto old_depth = suite_context.depth;
				auto old_index = suite_context.index;

				if(suite_context.depth == LITMUS_MAX_DEPTH)
					throw std::runtime_error(
						"reached maximum section depths available for tests, please increase 'LITMUS_MAX_DEPTH'");
				suite_context.depth += 1;
				suite_context.index = 0;

				suite_context.working_stack.set(m_Depth, m_Index);
				suite_context.output.scope_open(name, suite_context.working_stack, location,
												std::vector<std::string>{std::to_string(values)...});

				if constexpr(sizeof...(InvokeTypes) > 0)
					fn.template operator()<InvokeTypes...>(std::forward<Ts>(values)...);
				else
					fn(std::forward<Ts>(values)...);

				suite_context.output.scope_close();
				suite_context.working_stack.resize(m_Depth - 1);

				if(!suite_context.bail)
				{
					suite_context.stack.clear();
					suite_context.bail = true;
				}
				else if(!suite_context.stack.empty())
				{
					suite_context.stack.set(m_Depth, m_Index);
				}

				suite_context.depth = old_depth;
				suite_context.index = old_index;
			}

			size_t m_Depth{};
			LITMUS_MAX_TEST_ID_TYPE m_Index{};
		};
	} // namespace internal


	template <fixed_string Name, fixed_string... Categories>
	[[nodiscard]] constexpr auto section(const source_location& location = source_location::current())
	{
		return scope_t<section_functor>(Name, {Categories...}, location);
	}

	template <fixed_string Name, fixed_string... Categories, typename T0>
	requires(IsCopyConstructible<T0>)
		[[nodiscard]] constexpr auto section(T0&& v0, const source_location& location = source_location::current())
	{
		return scope_t<section_functor, T0>(Name, {Categories...}, location, std::forward<T0>(v0));
	}

	template <fixed_string Name, fixed_string... Categories, typename T0, typename T1>
	requires(IsCopyConstructible<T0, T1>)
		[[nodiscard]] constexpr auto section(T0&& v0, T1&& v1,
											 const source_location& location = source_location::current())
	{
		return scope_t<section_functor, T0, T1>(Name, {Categories...}, location, std::forward<T0>(v0),
												std::forward<T1>(v1));
	}

	template <fixed_string Name, fixed_string... Categories, typename T0, typename T1, typename T2>
	requires(IsCopyConstructible<T0, T1, T2>)
		[[nodiscard]] constexpr auto section(T0&& v0, T1&& v1, T2&& v2,
											 const source_location& location = source_location::current())
	{
		return scope_t<section_functor, T0, T1, T2>(Name, {Categories...}, location, std::forward<T0>(v0),
													std::forward<T1>(v1), std::forward<T2>(v2));
	}

	template <fixed_string Name, fixed_string... Categories, typename T0, typename T1, typename T2, typename T3>
	requires(IsCopyConstructible<T0, T1, T2, T3>)
		[[nodiscard]] constexpr auto section(T0&& v0, T1&& v1, T2&& v2, T3&& v3,
											 const source_location& location = source_location::current())
	{
		return scope_t<section_functor, T0, T1, T2, T3>(Name, {Categories...}, location, std::forward<T0>(v0),
														std::forward<T1>(v1), std::forward<T2>(v2),
														std::forward<T3>(v3));
	}
	template <fixed_string Name, fixed_string... Categories, typename T0, typename T1, typename T2, typename T3,
			  typename T4>
	requires(IsCopyConstructible<T0, T1, T2, T3, T4>)
		[[nodiscard]] constexpr auto section(T0&& v0, T1&& v1, T2&& v2, T3&& v3, T4&& v4,
											 const source_location& location = source_location::current())
	{
		return scope_t<section_functor, T0, T1, T2, T3, T4>(Name, {Categories...}, location, std::forward<T0>(v0),
															std::forward<T1>(v1), std::forward<T2>(v2),
															std::forward<T3>(v3), std::forward<T4>(v4));
	}
	template <fixed_string Name, fixed_string... Categories, typename T0, typename T1, typename T2, typename T3,
			  typename T4, typename T5>
	requires(IsCopyConstructible<T0, T1, T2, T3, T4, T5>)
		[[nodiscard]] constexpr auto section(T0&& v0, T1&& v1, T2&& v2, T3&& v3, T4&& v4, T5&& v5,
											 const source_location& location = source_location::current())
	{
		return scope_t<section_functor, T0, T1, T2, T3, T4, T5>(
			Name, {Categories...}, location, std::forward<T0>(v0), std::forward<T1>(v1), std::forward<T2>(v2),
			std::forward<T3>(v3), std::forward<T4>(v4), std::forward<T5>(v5));
	}
	template <fixed_string Name, fixed_string... Categories, typename T0, typename T1, typename T2, typename T3,
			  typename T4, typename T5, typename T6>
	requires(IsCopyConstructible<T0, T1, T2, T3, T4, T5, T6>)
		[[nodiscard]] constexpr auto section(T0&& v0, T1&& v1, T2&& v2, T3&& v3, T4&& v4, T5&& v5, T6&& v6,
											 const source_location& location = source_location::current())
	{
		return scope_t<section_functor, T0, T1, T2, T3, T4, T5, T6>(
			Name, {Categories...}, location, std::forward<T0>(v0), std::forward<T1>(v1), std::forward<T2>(v2),
			std::forward<T3>(v3), std::forward<T4>(v4), std::forward<T5>(v5), std::forward<T6>(v6));
	}

	template <fixed_string Name, fixed_string... Categories, typename T0, typename T1, typename T2, typename T3,
			  typename T4, typename T5, typename T6, typename T7>
	requires(IsCopyConstructible<T0, T1, T2, T3, T4, T5, T6, T7>)
		[[nodiscard]] constexpr auto section(T0&& v0, T1&& v1, T2&& v2, T3&& v3, T4&& v4, T5&& v5, T6&& v6, T7&& v7,
											 const source_location& location = source_location::current())
	{
		return scope_t<section_functor, T0, T1, T2, T3, T4, T5, T6, T7>(
			Name, {Categories...}, location, std::forward<T0>(v0), std::forward<T1>(v1), std::forward<T2>(v2),
			std::forward<T3>(v3), std::forward<T4>(v4), std::forward<T5>(v5), std::forward<T6>(v6),
			std::forward<T7>(v7));
	}
} // namespace litmus
