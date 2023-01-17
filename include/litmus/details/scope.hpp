#pragma once
#include <functional>
#include <stdexcept>
#include <typeinfo>
#include <tuple>

#include <litmus/details/context.hpp>
#include <litmus/details/source_location.hpp>

namespace litmus
{
	template <typename... Ts>
	struct tpack;
	template <auto... Values>
	struct vpack;


	inline namespace internal
	{
		template <typename T>
		concept SupportsGenerators = requires { T::supports_generators == true; };

		template <typename T>
		concept SupportsTemplates = requires { T::supports_templates == true; };

		template <typename T>
		struct is_tpack : std::false_type
		{};
		template <typename... Ts>
		struct is_tpack<tpack<Ts...>> : std::true_type
		{};
		template <typename T>
		struct is_vpack : std::false_type
		{};
		template <auto... Ts>
		struct is_vpack<vpack<Ts...>> : std::true_type
		{};

		template <auto Value>
		struct vpack_value
		{
			using type = decltype(Value);
			static constexpr auto value{Value};
		};

		template <typename T>
		concept IsTemplatePack = is_vpack<std::remove_cvref_t<T>>::value || is_tpack<std::remove_cvref_t<T>>::value;
	} // namespace internal

	template <typename... Ts>
	struct tpack
	{
		static constexpr auto size = sizeof...(Ts);

		using tuple_t = std::tuple<Ts...>;

		template <size_t N>
		using type = std::tuple_element_t<N, tuple_t>;
	};

	template <auto... Values>
	struct vpack
	{
		static constexpr auto size = sizeof...(Values);

		using tuple_t = std::tuple<vpack_value<Values>...>;

		template <size_t N>
		using type = std::tuple_element_t<N, tuple_t>;
	};

	inline namespace internal
	{
		template <typename T>
		concept IsGenerator = requires { T::is_generator; };

		template <typename T>
		concept HasToString = requires(std::remove_cvref_t<T> t) { std::to_string(t); };

		template <HasToString T>
		auto stringify(T&& val) -> std::string
		{
			return std::to_string(val);
		}

		template <typename T>
		auto stringify(T&&) -> std::string
		{
			return std::string{strtype::stringify_typename<T>()};
		}

		template <typename T, size_t... N>
		auto pack_to_string_impl(const T& values, std::index_sequence<N...>) -> std::vector<std::string>
		{
			return {stringify(std::get<N>(values))...};
		}
		template <size_t N>
		auto pack_to_string(const auto& values) -> std::vector<std::string>
		{
			return pack_to_string_impl(values, std::make_index_sequence<N>{});
		}

		template <typename T>
		[[nodiscard]] constexpr auto generator_size(const T& generator) noexcept -> size_t
		{
			if constexpr(IsGenerator<T>)
			{
				return generator.size();
			}
			else
			{
				return 1u;
				/* code */
			}
		}

		template <typename tuple_t>
		class scope_args_container
		{
		  protected:
			tuple_t m_ScopeArgs{};

			template <typename... Ys>
			constexpr scope_args_container(Ys&&... args) : m_ScopeArgs(std::forward<Ys>(args)...)
			{}

			constexpr void unpack_args(auto&& fn) const { unpack_args_impl0<0>(fn); }

		  private:
			template <size_t Index, typename... Ts>
			constexpr void unpack_args_impl1(auto& fn, Ts&&... values) const
			{
				using fn_type = std::tuple_element_t<Index, tuple_t>;
				if constexpr(IsGenerator<fn_type>)
				{
					auto generator{std::get<Index>(m_ScopeArgs)};

					for(auto i = 0u; i < generator.size(); ++i)
					{
						unpack_args_impl0<Index + 1>(fn, std::forward<Ts>(values)..., generator.next());
					}
				}
				else
				{
					unpack_args_impl0<Index + 1>(fn, std::forward<Ts>(values)..., std::get<Index>(m_ScopeArgs));
				}
			}

			// unpack the argument pack, detect generators
			template <size_t Index, typename... Ts>
			constexpr void unpack_args_impl0(auto& fn, Ts&&... values) const
			{
				if constexpr(std::tuple_size_v<tuple_t> == Index)
				{
					fn(std::forward<Ts>(values)...);
				}
				else
				{
					unpack_args_impl1<Index>(fn, std::forward<Ts>(values)...);
				}
			}
		};

		template <typename Functor, typename tuple_t, typename... FnTypes>
		class templated_scope_t : public scope_args_container<tuple_t>
		{
			using args_base = scope_args_container<tuple_t>;

			template <size_t Index, typename... InvokeTypes, size_t... N>
			constexpr void unpack_templates_impl1(auto& fn, std::index_sequence<N...>)
			{
				using fn_type = std::tuple_element_t<Index, std::tuple<FnTypes...>>;

				if constexpr(Index + 1 == sizeof...(FnTypes))
				{
					(args_base::unpack_args([&fn, name = m_Name, location = m_Location, &scope = m_ScopeObject,
											 &categories = m_Categories](auto&&... values) {
						 scope.template operator()<InvokeTypes..., typename fn_type::template type<N>>(
							 fn, name, location, categories, std::forward<decltype(values)>(values)...);
					 }),
					 ...);
				}
				else
				{
					(unpack_templates_impl0<Index + 1, InvokeTypes..., typename fn_type::template type<N>>(fn), ...);
				}
			}

			template <size_t Index, typename... InvokeTypes>
			constexpr void unpack_templates_impl0(auto& fn)
			{
				using fn_type = std::tuple_element_t<Index, std::tuple<FnTypes...>>;
				unpack_templates_impl1<Index, InvokeTypes...>(fn, std::make_index_sequence<fn_type::size>{});
			}

		  public:
			constexpr templated_scope_t(Functor&& scope, const char* name, std::vector<const char*>&& categories,
										tuple_t&& values, const source_location& location)
				: args_base(std::move(values)), m_Name(name), m_Categories(std::move(categories)), m_Location(location),
				  m_ScopeObject(std::move(scope))
			{}

			template <typename Fn>
			constexpr auto operator=(Fn&& fn) -> templated_scope_t&
			{
				if(m_HasRun || !m_ScopeObject.should_run()) return *this;
				unpack_templates_impl0<0>(fn);
				m_HasRun = true;
				return *this;
			}

			constexpr operator bool() const noexcept { return m_HasRun; }

		  private:
			const char* m_Name;
			std::vector<const char*> m_Categories;
			const source_location& m_Location;
			Functor m_ScopeObject{};
			bool m_HasRun{false};
		};

		template <typename Functor, typename... Ts>
		class scope_t : public scope_args_container<std::tuple<Ts...>>
		{
			using tuple_t = std::tuple<Ts...>;

			using args_base = scope_args_container<std::tuple<Ts...>>;

		  public:
			template <typename... Ys>
			scope_t(const char* name, std::vector<const char*> categories, const source_location& location,
					Ys&&... values)
				: args_base(std::forward<Ys>(values)...), m_Name(name), m_Categories(categories), m_Location(location)
			{}
			const char* name() noexcept { return m_Name; }

			template <typename Fn>
			scope_t& operator=(Fn&& fn)
			{
				return test(std::forward<Fn>(fn));
			}

			template <typename Fn>
			scope_t& test(Fn&& fn)
			{
				if(m_HasRun || !m_ScopeObject.should_run()) return *this;
				try
				{
					if constexpr(SupportsGenerators<Functor>)
					{
						args_base::unpack_args([&fn, name = m_Name, location = m_Location, &scope = m_ScopeObject,
												&categories = m_Categories](auto&&... values) {
							scope.template operator()<>(fn, name, location, categories,
														std::forward<decltype(values)>(values)...);
						});
					}
					else
					{
						std::apply(
							[&fn, name = m_Name, location = m_Location, &scope = m_ScopeObject,
							 &categories = m_Categories](auto&&... values) {
								static_assert((!IsGenerator<std::remove_cvref_t<decltype(values)>> && ...),
											  "generator types not supported on sections.");
								scope.template operator()<>(fn, name, location, categories,
															std::forward<decltype(values)>(values)...);
							},
							args_base::m_ScopeArgs);
					}
				}
				catch(const std::exception& e)
				{
					std::cerr << "Exception logged in scope: " << m_Name << std::endl;
					std::cerr << "message: " << e.what() << std::endl;
					throw e;
				}
				catch(...)
				{
					std::cerr << "Exception logged in scope: " << m_Name << std::endl;
					std::rethrow_exception(std::current_exception());
				}
				m_HasRun = true;
				return *this;
			}

			template <IsTemplatePack Y, IsTemplatePack... Ys>
				requires(SupportsTemplates<Functor>)
			auto templates() -> templated_scope_t<Functor, tuple_t, Y, Ys...>
			{
				return templated_scope_t<Functor, tuple_t, Y, Ys...>{std::move(m_ScopeObject), m_Name,
																	 std::move(m_Categories),
																	 std::move(args_base::m_ScopeArgs), m_Location};
			}

			constexpr operator bool() const noexcept { return m_HasRun; }

		  private:
			const char* m_Name;
			std::vector<const char*> m_Categories;
			const source_location& m_Location;
			Functor m_ScopeObject{};
			bool m_HasRun{false};
		}; // namespace internal

		template <typename... Ts>
		concept IsCopyConstructible = (std::is_copy_constructible_v<Ts> && ...);
	} // namespace internal
} // namespace litmus
