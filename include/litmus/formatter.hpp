#pragma once

#include <litmus/details/source_location.hpp>
#include <litmus/details/test_result.hpp>

namespace litmus
{
	class formatter
	{
	  public:
		formatter()					= default;
		virtual ~formatter()		= default;
		formatter(const formatter&) = default;
		formatter(formatter&&)		= default;

		auto operator=(const formatter&) -> formatter& = default;
		auto operator=(formatter&&) -> formatter& = default;

		virtual void suite_begin(const test_result_t::scope_t& scope, [[maybe_unused]] const source_location& location)
		{
			scope_begin(scope);
		}
		virtual void suite_end(const test_result_t::scope_t& scope, [[maybe_unused]] const source_location& location)
		{
			scope_end(scope);
		}

		virtual void scope_begin([[maybe_unused]] const test_result_t::scope_t& scope) {}

		virtual void scope_end([[maybe_unused]] const test_result_t::scope_t& scope) {}

		virtual void expect([[maybe_unused]] const test_result_t::expect_t& expect,
							[[maybe_unused]] const test_result_t::scope_t& scope)
		{}

		virtual void write_totals([[maybe_unused]] size_t pass, [[maybe_unused]] size_t fail,
								  [[maybe_unused]] size_t fatal)
		{}

	  private:
	};
} // namespace litmus