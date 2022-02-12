#pragma once
#include <cstddef>
#include <litmus/details/test_result.hpp>

namespace litmus
{
	inline namespace internal
	{
		extern thread_local struct suite_context_t
		{
			void reset()
			{
				depth		  = 0;
				index		  = 0;
				stack		  = {};
				working_stack = {};
				bail		  = false;
			}
			size_t depth{0};
			size_t index{0};
			test_result_t output{};
			test_id_t stack{};
			test_id_t working_stack{};
			bool bail{false};
			bool is_benchmark{false};
		} suite_context;
	} // namespace internal
} // namespace litmus