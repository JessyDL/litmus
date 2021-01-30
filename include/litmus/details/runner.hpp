#pragma once
#include <functional>
#include <vector>
#include <unordered_map>

#include <litmus/details/test_result.hpp>

namespace litmus
{
	inline namespace internal
	{
		struct runner_t
		{
		  public:
			using test_t = std::unordered_map<std::string, std::vector<std::function<test_result_t()>>>;

			runner_t()
			{
				if(m_RefCount == 0)
				{
					m_Tests		 = new std::vector<std::function<test_result_t()>>();
					m_NamedTests = new std::unordered_map<const char*, test_t>();
				}
				++m_RefCount;
			}

			~runner_t()
			{
				if(--m_RefCount == 0)
				{
					delete(m_Tests);
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

			// template <typename T>
			// static void test(T&& arg)
			// {
			// 	m_Tests->emplace_back(std::forward<T>(arg));
			// }

			template <typename T>
			static void test(const char* name, std::string templates, T&& arg)
			{
				(*m_NamedTests)[name][templates].emplace_back(std::forward<T>(arg));
			}

			template <typename T>
			static void benchmark(T&& arg)
			{
				m_Benchmarks->emplace_back(std::forward<T>(arg));
			}

		  private:
			static std::unordered_map<const char*, test_t>* m_NamedTests;
			static std::vector<std::function<test_result_t()>>* m_Tests;
			static std::vector<std::function<void()>>* m_Benchmarks;
			static unsigned m_RefCount;
		};

		runner_t const runner;
	} // namespace internal
} // namespace litmus
