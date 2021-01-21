#pragma once
#include <functional>
#include <vector>

#include <litmus/details/test_result.hpp>

namespace litmus
{
	inline namespace internal
	{
		struct runner_t
		{
		  public:
			runner_t()
			{
				if(m_RefCount == 0) m_Tests = new std::vector<std::function<test_result_t()>>();
				++m_RefCount;
			}

			~runner_t()
			{
				if(--m_RefCount == 0) delete(m_Tests);
			}

			runner_t(runner_t const&) = delete;
			runner_t(runner_t&&)	  = delete;

			auto operator=(runner_t const&) -> runner_t& = delete;
			auto operator=(runner_t&&) -> runner_t& = delete;

			operator std::vector<std::function<test_result_t()>>&() const { return *m_Tests; }
			auto operator->() const -> std::vector<std::function<test_result_t()>>* { return m_Tests; }
			auto operator*() const -> std::vector<std::function<test_result_t()>>& { return *m_Tests; }

			[[nodiscard]] static auto begin() noexcept { return m_Tests->begin(); }
			[[nodiscard]] static auto cbegin() noexcept { return m_Tests->cbegin(); }

			[[nodiscard]] static auto end() noexcept { return m_Tests->end(); }
			[[nodiscard]] static auto cend() noexcept { return m_Tests->cend(); }

			[[nodiscard]] static auto size() noexcept { return m_Tests->size(); }

			template <typename T>
			static void test(T&& arg)
			{
				m_Tests->emplace_back(std::forward<T>(arg));
			}

			template <typename T>
			static void benchmark(T&& arg)
			{
				m_Benchmarks->emplace_back(std::forward<T>(arg));
			}

		  private:
			static std::vector<std::function<test_result_t()>>* m_Tests;
			static std::vector<std::function<void()>>* m_Benchmarks;
			static unsigned m_RefCount;
		};

		runner_t const runner;
	} // namespace internal
} // namespace litmus
