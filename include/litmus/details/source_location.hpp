#pragma once

/*
	replacement std::source_location stub for when the target platform lacks an implementation.
*/
#if __has_include(<source_location>)
#include <source_location>
#elif __has_include(<experimental/source_location>)
#include <experimental/source_location>
#else
#error source_location is necesary
#endif


namespace litmus
{
	inline namespace internal
	{
#if __has_include(<source_location>)
		using source_location = std::source_location;
#elif __has_include(<experimental/source_location>)
		using source_location = std::experimental::source_location;
#endif
	} // namespace internal
} // namespace litmus