#pragma once

namespace litmus::generator
{
	struct range_plus
	{
		template <typename T>
		static constexpr auto next(const T& lhs, const T& rhs) noexcept
		{
			return lhs + rhs;
		}

		template <typename T>
		static constexpr auto validate(const T& lhs, const T& rhs, const T& steps) noexcept
		{
			return ((rhs - lhs) % steps) == 0;
		}

		template <typename T>
		static constexpr auto size(const T& lhs, const T& rhs, const T& steps) noexcept
		{
			return ((rhs - lhs) / steps) + 1;
		}
	};

	template <typename T, T Min, T Max, T Increment = T{1}, typename Operation = range_plus>
	class range
	{
		static_assert(Operation::validate(Min, Max, Increment), "Increment should be a multiple of [Max - Min]");

	  public:
		constexpr static bool is_generator{true};

		constexpr operator size_t() const noexcept { return m_Current; }

		constexpr auto size() const noexcept { return Operation::size(Min, Max, Increment); }

		constexpr auto next() noexcept
		{
			auto curr = m_Current;
			m_Current = Operation::next(m_Current, Increment);
			return curr;
		}

	  private:
		T m_Current{Min};
	};

	template <auto Value0, decltype(Value0)... Values>
	class array
	{
		using type = decltype(Value0);

	  public:
		constexpr static bool is_generator{true};

		constexpr operator type() const noexcept { return m_Values[m_Current]; }

		constexpr auto size() const noexcept { return sizeof...(Values) + 1; }

		constexpr auto next() noexcept
		{
			m_Current += 1;
			return m_Values[m_Current - 1];
		}

	  private:
		static constexpr type m_Values[]{Value0, Values...};
		size_t m_Current{0};
	};
} // namespace litmus::generator