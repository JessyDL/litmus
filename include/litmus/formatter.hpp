#pragma once
#include <chrono>
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
		auto operator=(formatter &&) -> formatter& = default;

		virtual void suite_begin([[maybe_unused]] const char* name, [[maybe_unused]] size_t pass,
								 [[maybe_unused]] size_t fail, [[maybe_unused]] size_t fatal,
								 [[maybe_unused]] const source_location& location,
								 [[maybe_unused]] std::chrono::microseconds duration)
		{}
		virtual void suite_end([[maybe_unused]] const char* name, [[maybe_unused]] size_t pass,
							   [[maybe_unused]] size_t fail, [[maybe_unused]] size_t fatal,
							   [[maybe_unused]] const source_location& location,
							   [[maybe_unused]] std::chrono::microseconds duration)
		{}


		virtual void suite_iterate_templates([[maybe_unused]] std::string_view templates) {}
		virtual void suite_iterate_parameters([[maybe_unused]] const std::vector<std::string>& parameters) {}

		virtual void scope_begin([[maybe_unused]] const test_result_t::scope_t& scope) {}

		virtual void scope_end([[maybe_unused]] const test_result_t::scope_t& scope) {}

		virtual void expect([[maybe_unused]] const test_result_t::expect_t& expect,
							[[maybe_unused]] const test_result_t::scope_t& scope)
		{}

		virtual void write_totals([[maybe_unused]] size_t pass, [[maybe_unused]] size_t fail,
								  [[maybe_unused]] size_t fatal, [[maybe_unused]] std::chrono::microseconds duration,
								  [[maybe_unused]] std::chrono::microseconds user_duration)
		{}

	  private:
	};
} // namespace litmus
