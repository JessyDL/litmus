#pragma once
#include <litmus/details/utility.hpp>

namespace litmus::generator
{
	template <typename... Ts>
	class typelist
	{
		using pack = type_pack<Ts...>;

	  public:
		constexpr auto size() const noexcept { return sizeof...(Ts); }
		constexpr auto next() noexcept { return m_Current += Increment; }

	  private:
		size_t m_Index{0};
	};
} // namespace litmus::generator