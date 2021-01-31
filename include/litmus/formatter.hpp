#pragma once
#include <chrono>
#include <ostream>
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

		virtual void begin(size_t){};

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
		virtual void end(){};
		void set_stream(std::ostream& stream, bool is_console)
		{
			m_Output	= &stream;
			m_IsConsole = is_console;
		}

		void flush() { output() << std::flush; }

	  protected:
		std::ostream& output() { return *m_Output; }
		bool is_console() const noexcept { return m_IsConsole; }

		std::ostream* m_Output;
		bool m_IsConsole{true};
	};
} // namespace litmus
