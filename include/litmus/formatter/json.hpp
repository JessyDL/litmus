#pragma once
#include <litmus/formatter.hpp>

namespace litmus::formatters
{
	class json final : public litmus::formatter
	{
	  public:
		void begin(size_t tests) override { m_Tests = tests; }
		void suite_begin(const char* name, size_t pass, size_t fail, size_t fatal, const source_location& location,
						 std::chrono::microseconds duration) override
		{
			if(m_Iteration == 0u) output() << "[";
			output() << "{\n\t\"name\": \"" << name << "\",\n\t\"pass\": " << std::to_string(pass)
					 << ",\n\t\"fail\": " << std::to_string(fail) << ",\n\t\"fatal\": " << std::to_string(fatal)
					 << ",\n\t\"source\": \"" << location.file_name() << ":" << std::to_string(location.line())
					 << "\",\n\t\"duration_microseconds\": " << std::to_string(duration.count()) << ",\n\t\"tests\": [";
			++m_Iteration;
		}

		void suite_end(const char* name, size_t pass, size_t fail, size_t fatal, const source_location& location,
					   std::chrono::microseconds duration) override
		{
			if(m_Iteration == m_Tests)
				output() << "]\n}]\n";
			else
				output() << "]\n},\n";
		}

	  private:
		size_t m_Tests{0u};
		size_t m_Iteration{0u};
	};
} // namespace litmus::formatters
