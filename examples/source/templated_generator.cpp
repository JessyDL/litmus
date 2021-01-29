#include <litmus/litmus.hpp>

#include <litmus/expect.hpp>
#include <litmus/section.hpp>
#include <litmus/suite.hpp>

#include <litmus/generator/range.hpp>

using namespace litmus;
using namespace litmus::generator;

auto test0 = suite<"basic template test">().templates<tpack<float, int, bool>>() = []<typename T>()
{
	static_assert(std::is_same_v<T, bool> || std::is_same_v<T, int> || std::is_same_v<T, float>);
};

auto test1 = suite<"template matrix test">().templates<tpack<float, int>, tpack<char, bool, bool>>() =
	[]<typename T0, typename T1>(){

	};

auto test3 = suite<"NTTP test">().templates<tpack<float, int>>() = []<typename T>()
{
	expect(T{1}) == T{1};
	static_assert(std::is_same_v<T, int> || std::is_same_v<T, float>);
};

auto test4 = suite<"NTTP + template test">().templates<tpack<float, int>, vpack<0, 5>>() =
	[]<typename T, typename value_t>()
{
	static constexpr auto value = value_t::value;
	static_assert(std::is_same_v<T, int> || std::is_same_v<T, float>);
	static_assert(value == 0 || value == 5);
};

auto test5 =
	suite<"generator + NTTP + template test">(range<size_t, 0, 3, 1>{}).templates<tpack<float, int>, vpack<0, 5>>() =
		[]<typename T, typename value_t>(auto gen_value)
{
	static constexpr auto value = value_t::value;
	static_assert(std::is_same_v<T, int> || std::is_same_v<T, float>);
	static_assert(value == 0 || value == 5);

	expect(gen_value) == gen_value;
};
