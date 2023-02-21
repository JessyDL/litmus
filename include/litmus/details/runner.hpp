#pragma once
#include <functional>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include <litmus/details/test_result.hpp>
#include <litmus/details/utility.hpp>

namespace litmus
{

	inline namespace internal
	{

		template <typename... Ts>
		constexpr const std::uintptr_t uuid_var{0u};

		template <typename... Ts>
		constexpr const std::uintptr_t* uuid() noexcept
		{
			return &uuid_var<Ts...>;
		}


		using uuid_t = const std::uintptr_t* (*)();

		template <typename... Ts>
		constexpr uuid_t uuid_for()
		{
			return uuid<typename std::decay<Ts>::type...>;
		}

		struct runner_t
		{
		  public:
			struct template_pack_t
			{
				std::vector<std::string> templates{};
				std::vector<std::function<test_result_t()>> functions{};
			};

			using test_t			  = std::unordered_map<uuid_t, template_pack_t>;
			runner_t()				  = default;
			runner_t(runner_t const&) = delete;
			runner_t(runner_t&&)	  = delete;

			auto operator=(runner_t const&) -> runner_t& = delete;
			auto operator=(runner_t&&) -> runner_t&		 = delete;

			[[nodiscard]] auto begin() noexcept { return m_NamedTests.begin(); }
			[[nodiscard]] auto begin() const noexcept { return m_NamedTests.begin(); }
			[[nodiscard]] auto cbegin() const noexcept { return m_NamedTests.cbegin(); }

			[[nodiscard]] auto end() noexcept { return m_NamedTests.end(); }
			[[nodiscard]] auto end() const noexcept { return m_NamedTests.cend(); }
			[[nodiscard]] auto cend() const noexcept { return m_NamedTests.cend(); }

			[[nodiscard]] auto size() const noexcept { return m_NamedTests.size(); }

			template <typename... Ts>
			void test(const char* name, auto&& arg)
			{
				if(auto it = m_NamedTests.find(name); it == std::end(m_NamedTests))
				{
					m_NamedTests.emplace(name, test_t{});
				}
				auto& test = m_NamedTests[name];
				auto& t	   = test[uuid_for<Ts...>()];
				if constexpr(sizeof...(Ts) > 0)
				{
					if(t.templates.size() != sizeof...(Ts))
						t.templates = std::vector<std::string>{{type_to_name_internal<Ts>()}...};
				}
				t.functions.emplace_back(std::forward<decltype(arg)>(arg));
			}

			template <typename T>
			void benchmark(T&& arg)
			{
				m_Benchmarks->emplace_back(std::forward<T>(arg));
			}

		  private:
			std::unordered_map<const char*, test_t> m_NamedTests;
			std::vector<std::function<void()>> m_Benchmarks;
		};

		extern runner_t runner;
	} // namespace internal
} // namespace litmus
