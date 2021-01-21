#pragma once

namespace litmus
{
	inline namespace internal
	{
#ifndef LITMUS_NOEXCEPT
		constexpr auto except(bool condition, auto&& exception) -> bool
		{
			if(condition)
			{
				throw std::move(exception);
			}
			return false;
		}
		constexpr auto except(auto&& exception) -> bool
		{
			throw exception;
			return false;
		}
#else
		constexpr auto except(bool condition, auto&&) noexcept -> bool { return condition; }
		constexpr auto except(auto&&) noexcept -> bool { return true; }
#endif
	} // namespace internal
} // namespace litmus