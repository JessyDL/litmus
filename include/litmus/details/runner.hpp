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

			using test_t = std::unordered_map<uuid_t, template_pack_t>;

			runner_t()
			{
				if(m_RefCount == 0)
				{
					m_NamedTests = new std::unordered_map<const char*, test_t>();
				}
				++m_RefCount;
			}

			~runner_t()
			{
				if(--m_RefCount == 0)
				{
					delete(m_NamedTests);
				}
			}

			runner_t(runner_t const&) = delete;
			runner_t(runner_t&&)	  = delete;

			auto operator=(runner_t const&) -> runner_t& = delete;
			auto operator=(runner_t &&) -> runner_t& = delete;

			[[nodiscard]] static auto begin() noexcept { return m_NamedTests->begin(); }
			[[nodiscard]] static auto cbegin() noexcept { return m_NamedTests->cbegin(); }

			[[nodiscard]] static auto end() noexcept { return m_NamedTests->end(); }
			[[nodiscard]] static auto cend() noexcept { return m_NamedTests->cend(); }

			[[nodiscard]] static auto size() noexcept { return m_NamedTests->size(); }

			template <typename... Ts>
			static void test(const char* name, auto&& arg)
			{
				auto& t = (*m_NamedTests)[name][uuid_for<Ts...>()];
				if constexpr(sizeof...(Ts) > 0)
				{
					if(t.templates.size() != sizeof...(Ts))
						t.templates = std::vector<std::string>{{type_to_name_internal<Ts>()}...};
				}
				t.functions.emplace_back(std::forward<decltype(arg)>(arg));
			}

			template <typename T>
			static void benchmark(T&& arg)
			{
				m_Benchmarks->emplace_back(std::forward<T>(arg));
			}

		  private:
			static std::unordered_map<const char*, test_t>* m_NamedTests;
			static std::vector<std::function<void()>>* m_Benchmarks;
			static unsigned m_RefCount;
		};

		runner_t const runner;
	} // namespace internal
} // namespace litmus
