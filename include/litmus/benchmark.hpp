#pragma once
#include <tuple>

#include <litmus/details/fixed_string.hpp>

namespace litmus
{
	inline namespace internal
	{
		template <typename... Ts>
		struct benchmark_t
		{
			template <typename... Ys>
			benchmark_t(const char* name, Ys&&... ys) : m_Name(name), m_Args(std::forward<Ys>(ys)...)
			{}

			template <typename Fn>
			requires(requires(Fn fn, std::tuple<Ts...> values) { std::apply(fn, values); }) auto operator=(Fn&& fn)
				-> benchmark_t
			{
				std::apply(fn, m_Args);
				return *this;
			}

			const char* m_Name;
			std::tuple<Ts...> m_Args;
		};

	} // namespace internal


	template <fixed_string Name, typename... Ts>
	constexpr auto benchmark(Ts&&... args) -> benchmark_t<Ts...>
	{
		return benchmark_t<Ts...>{Name, std::forward<Ts>(args)...};
	}
} // namespace litmus