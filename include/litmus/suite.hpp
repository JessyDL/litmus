#pragma once
#include <type_traits>

#include <litmus/details/fixed_string.hpp>
#include <litmus/details/runner.hpp>
#include <litmus/details/scope.hpp>
#include <litmus/details/source_location.hpp>

namespace litmus
{
	template <auto Value>
	inline auto type_to_name(std::type_identity<vpack_value<Value>>) -> std::string
	{
		return std::to_string(Value);
	}

	inline namespace internal
	{
		struct suite_functor
		{
			static constexpr bool supports_generators = true;
			static constexpr bool supports_templates  = true;

			bool should_run() noexcept { return true; }

			template <typename... InvokeTypes, typename... Ts>
			constexpr void operator()(auto& fn, const char* name, const source_location& location,
									  const std::vector<const char*>& categories, Ts&&... values)
			{
				runner.template test<InvokeTypes...>(name, [name = name, values = std::tuple{values...}, fn = fn,
															location = location, categories = categories]() {
					suite_context = {};

					if(config->categories.empty() ||
					   std::any_of(std::begin(categories), std::end(categories), [](const auto& category) {
						   return std::find(std::begin(config->categories), std::end(config->categories), category) !=
								  std::end(config->categories);
					   }))
					{
						static constexpr auto parameter_size = sizeof...(InvokeTypes);

						suite_context.output.scope_open(name, {}, location,
														pack_to_string<std::tuple_size_v<decltype(values)>>(values));
						test_id_t next_stack{};
						do
						{
							suite_context.reset();
							suite_context.stack = std::move(next_stack);
							if constexpr(parameter_size > 0)
							{
								std::apply(
									[&fn](auto&&... values) { fn.template operator()<InvokeTypes...>(values...); },
									values);
							}
							else
							{
								std::apply(fn, values);
							}

							next_stack = std::move(suite_context.stack);
						} while(!next_stack.empty() && !suite_context.output.fatal);

						suite_context.output.scope_close();
					}
					suite_context.output.sync();
					return suite_context.output;
				});
			}
		};
	} // namespace internal
	template <fixed_string Name, fixed_string... Categories>
	[[nodiscard]] constexpr auto suite(const source_location& location = source_location::current())
	{
		return scope_t<suite_functor>(Name, {Categories...}, location);
	}

	template <fixed_string Name, fixed_string... Categories, typename T0>
		requires(IsCopyConstructible<T0>)
	[[nodiscard]] constexpr auto suite(T0&& v0, const source_location& location = source_location::current())
	{
		return scope_t<suite_functor, T0>(Name, {Categories...}, location, std::forward<T0>(v0));
	}

	template <fixed_string Name, fixed_string... Categories, typename T0, typename T1>
		requires(IsCopyConstructible<T0, T1>)
	[[nodiscard]] constexpr auto suite(T0&& v0, T1&& v1, const source_location& location = source_location::current())
	{
		return scope_t<suite_functor, T0, T1>(Name, {Categories...}, location, std::forward<T0>(v0),
											  std::forward<T1>(v1));
	}

	template <fixed_string Name, fixed_string... Categories, typename T0, typename T1, typename T2>
		requires(IsCopyConstructible<T0, T1, T2>)
	[[nodiscard]] constexpr auto suite(T0&& v0, T1&& v1, T2&& v2,
									   const source_location& location = source_location::current())
	{
		return scope_t<suite_functor, T0, T1, T2>(Name, {Categories...}, location, std::forward<T0>(v0),
												  std::forward<T1>(v1), std::forward<T2>(v2));
	}

	template <fixed_string Name, fixed_string... Categories, typename T0, typename T1, typename T2, typename T3>
		requires(IsCopyConstructible<T0, T1, T2, T3>)
	[[nodiscard]] constexpr auto suite(T0&& v0, T1&& v1, T2&& v2, T3&& v3,
									   const source_location& location = source_location::current())
	{
		return scope_t<suite_functor, T0, T1, T2, T3>(Name, {Categories...}, location, std::forward<T0>(v0),
													  std::forward<T1>(v1), std::forward<T2>(v2), std::forward<T3>(v3));
	}
	template <fixed_string Name, fixed_string... Categories, typename T0, typename T1, typename T2, typename T3,
			  typename T4>
		requires(IsCopyConstructible<T0, T1, T2, T3, T4>)
	[[nodiscard]] constexpr auto suite(T0&& v0, T1&& v1, T2&& v2, T3&& v3, T4&& v4,
									   const source_location& location = source_location::current())
	{
		return scope_t<suite_functor, T0, T1, T2, T3, T4>(Name, {Categories...}, location, std::forward<T0>(v0),
														  std::forward<T1>(v1), std::forward<T2>(v2),
														  std::forward<T3>(v3), std::forward<T4>(v4));
	}
	template <fixed_string Name, fixed_string... Categories, typename T0, typename T1, typename T2, typename T3,
			  typename T4, typename T5>
		requires(IsCopyConstructible<T0, T1, T2, T3, T4, T5>)
	[[nodiscard]] constexpr auto suite(T0&& v0, T1&& v1, T2&& v2, T3&& v3, T4&& v4, T5&& v5,
									   const source_location& location = source_location::current())
	{
		return scope_t<suite_functor, T0, T1, T2, T3, T4, T5>(
			Name, {Categories...}, location, std::forward<T0>(v0), std::forward<T1>(v1), std::forward<T2>(v2),
			std::forward<T3>(v3), std::forward<T4>(v4), std::forward<T5>(v5));
	}
	template <fixed_string Name, fixed_string... Categories, typename T0, typename T1, typename T2, typename T3,
			  typename T4, typename T5, typename T6>
		requires(IsCopyConstructible<T0, T1, T2, T3, T4, T5, T6>)
	[[nodiscard]] constexpr auto suite(T0&& v0, T1&& v1, T2&& v2, T3&& v3, T4&& v4, T5&& v5, T6&& v6,
									   const source_location& location = source_location::current())
	{
		return scope_t<suite_functor, T0, T1, T2, T3, T4, T5, T6>(
			Name, {Categories...}, location, std::forward<T0>(v0), std::forward<T1>(v1), std::forward<T2>(v2),
			std::forward<T3>(v3), std::forward<T4>(v4), std::forward<T5>(v5), std::forward<T6>(v6));
	}

	template <fixed_string Name, fixed_string... Categories, typename T0, typename T1, typename T2, typename T3,
			  typename T4, typename T5, typename T6, typename T7>
		requires(IsCopyConstructible<T0, T1, T2, T3, T4, T5, T6, T7>)
	[[nodiscard]] constexpr auto suite(T0&& v0, T1&& v1, T2&& v2, T3&& v3, T4&& v4, T5&& v5, T6&& v6, T7&& v7,
									   const source_location& location = source_location::current())
	{
		return scope_t<suite_functor, T0, T1, T2, T3, T4, T5, T6, T7>(
			Name, {Categories...}, location, std::forward<T0>(v0), std::forward<T1>(v1), std::forward<T2>(v2),
			std::forward<T3>(v3), std::forward<T4>(v4), std::forward<T5>(v5), std::forward<T6>(v6),
			std::forward<T7>(v7));
	}

	template <typename TName, fixed_string... Categories>
	[[nodiscard]] constexpr auto suite(const source_location& location = source_location::current())
	{
		constexpr fixed_string name{strtype::stringify_typename<TName>().buf};
		return suite<name, Categories...>(location);
	}

	template <typename TName, fixed_string... Categories, typename T0>
		requires(IsCopyConstructible<T0>)
	[[nodiscard]] constexpr auto suite(T0&& v0, const source_location& location = source_location::current())
	{
		constexpr fixed_string name{strtype::stringify_typename<TName>().buf};
		return suite<name, Categories...>(std::forward<T0>(v0), location);
	}

	template <typename TName, fixed_string... Categories, typename T0, typename T1>
		requires(IsCopyConstructible<T0, T1>)
	[[nodiscard]] constexpr auto suite(T0&& v0, T1&& v1, const source_location& location = source_location::current())
	{
		constexpr fixed_string name{strtype::stringify_typename<TName>().buf};
		return suite<name, Categories...>(std::forward<T0>(v0), std::forward<T1>(v1), location);
	}

	template <typename TName, fixed_string... Categories, typename T0, typename T1, typename T2>
		requires(IsCopyConstructible<T0, T1, T2>)
	[[nodiscard]] constexpr auto suite(T0&& v0, T1&& v1, T2&& v2,
									   const source_location& location = source_location::current())
	{
		constexpr fixed_string name{strtype::stringify_typename<TName>().buf};
		return suite<name, Categories...>(std::forward<T0>(v0), std::forward<T1>(v1), std::forward<T2>(v2), location);
	}

	template <typename TName, fixed_string... Categories, typename T0, typename T1, typename T2, typename T3>
		requires(IsCopyConstructible<T0, T1, T2, T3>)
	[[nodiscard]] constexpr auto suite(T0&& v0, T1&& v1, T2&& v2, T3&& v3,
									   const source_location& location = source_location::current())
	{
		constexpr fixed_string name{strtype::stringify_typename<TName>().buf};
		return suite<name, Categories...>(std::forward<T0>(v0), std::forward<T1>(v1), std::forward<T2>(v2),
										  std::forward<T3>(v3), location);
	}
	template <typename TName, fixed_string... Categories, typename T0, typename T1, typename T2, typename T3,
			  typename T4>
		requires(IsCopyConstructible<T0, T1, T2, T3, T4>)
	[[nodiscard]] constexpr auto suite(T0&& v0, T1&& v1, T2&& v2, T3&& v3, T4&& v4,
									   const source_location& location = source_location::current())
	{
		constexpr fixed_string name{strtype::stringify_typename<TName>().buf};
		return suite<name, Categories...>(std::forward<T0>(v0), std::forward<T1>(v1), std::forward<T2>(v2),
										  std::forward<T3>(v3), std::forward<T4>(v4), location);
	}
	template <typename TName, fixed_string... Categories, typename T0, typename T1, typename T2, typename T3,
			  typename T4, typename T5>
		requires(IsCopyConstructible<T0, T1, T2, T3, T4, T5>)
	[[nodiscard]] constexpr auto suite(T0&& v0, T1&& v1, T2&& v2, T3&& v3, T4&& v4, T5&& v5,
									   const source_location& location = source_location::current())
	{
		constexpr fixed_string name{strtype::stringify_typename<TName>().buf};
		return suite<name, Categories...>(std::forward<T0>(v0), std::forward<T1>(v1), std::forward<T2>(v2),
										  std::forward<T3>(v3), std::forward<T4>(v4), std::forward<T5>(v5), location);
	}
	template <typename TName, fixed_string... Categories, typename T0, typename T1, typename T2, typename T3,
			  typename T4, typename T5, typename T6>
		requires(IsCopyConstructible<T0, T1, T2, T3, T4, T5, T6>)
	[[nodiscard]] constexpr auto suite(T0&& v0, T1&& v1, T2&& v2, T3&& v3, T4&& v4, T5&& v5, T6&& v6,
									   const source_location& location = source_location::current())
	{
		constexpr fixed_string name{strtype::stringify_typename<TName>().buf};
		return suite<name, Categories...>(std::forward<T0>(v0), std::forward<T1>(v1), std::forward<T2>(v2),
										  std::forward<T3>(v3), std::forward<T4>(v4), std::forward<T5>(v5),
										  std::forward<T6>(v6), location);
	}

	template <typename TName, fixed_string... Categories, typename T0, typename T1, typename T2, typename T3,
			  typename T4, typename T5, typename T6, typename T7>
		requires(IsCopyConstructible<T0, T1, T2, T3, T4, T5, T6, T7>)
	[[nodiscard]] constexpr auto suite(T0&& v0, T1&& v1, T2&& v2, T3&& v3, T4&& v4, T5&& v5, T6&& v6, T7&& v7,
									   const source_location& location = source_location::current())
	{
		constexpr fixed_string name{strtype::stringify_typename<TName>().buf};
		return suite<name, Categories...>(std::forward<T0>(v0), std::forward<T1>(v1), std::forward<T2>(v2),
										  std::forward<T3>(v3), std::forward<T4>(v4), std::forward<T5>(v5),
										  std::forward<T6>(v6), std::forward<T7>(v7), location);
	}
} // namespace litmus
