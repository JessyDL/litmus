#define LITMUS_FULL
#include <litmus/litmus.hpp>
#include <litmus/suite.hpp>
#include <litmus/section.hpp>
#include <litmus/expect.hpp>
#include <litmus/benchmark.hpp>
#include <memory>


#include <litmus/generator/range.hpp>


using namespace litmus;
using namespace litmus::generator;

auto template_test = suite<"template_generator">().templates<tpack<int, float>>() = []<typename T>()
{
	static_assert(std::is_same_v<T, int> || std::is_same_v<T, float>);
};

auto template_test2 = suite<"2d_template_generator">().templates<tpack<int, float>, tpack<std::string, bool, char>>() =
	[]<typename T, typename Y>(){};

auto range_generator_test = suite<"range_generator">(range<size_t, 0, 9, 1>{}) = []([[maybe_unused]] size_t value) {
	std::vector<int> vec{};
	vec.resize(value);
	expect(vec.size()) == value;
};

auto multi_range_generator_test								= suite<"multi_range_generator">(
	range<size_t, 0, 9, 1>{}, range<size_t, 100, 109, 3>{}) = [](size_t value0, size_t value1) {
	std::vector<int> vec{};
	value0 += value1;
	vec.resize(value0);
	expect(vec.size()) == value0;
};

auto value_range_generator_test =
	suite<"value_range_generator">(array<0, 3, 8, 9>{}) = [](int value) { expect(value) != 1; };

auto basic_secion_test = suite<"basic">() = []() {
	expect(5) == 5;
	section<"name">() = []() {};
};

auto basic_test = suite<"basic", "cat0", "cat1">() = []() {
	expect(
		[&](bool this_is_a_really_long_name_to_upset_alignment) -> bool {
			return this_is_a_really_long_name_to_upset_alignment;
		},
		true) == true;
	expect(5) == 5;
	require(5) == 5;
	expect(5) != 5;
	require(5) != 5;
	expect(5) == 5;
};

auto fixture_test = suite<"vector", "cat1", "cat3">(5) = [](int value) {
	using type = std::vector<int>;
	type vec{};
	auto has_ran = section<"push_back">(value) = [&vec](int value) mutable {
		vec.push_back(value);
		info("element 0 should be ", value);
		require(vec[0]) == 5;

		section<"nested">() = [] {
			expect(5) == 5;
			expect(5) != 6;
		};
	};

	section<"empty_section">() = [] { expect(5) != 7; };

	info("vec.size() should be ", ((has_ran) ? 1u : 0u), " as section push_back ",
		 ((has_ran) ? "has executed" : "did not run"));
	expect(vec.size()) == ((has_ran) ? 1u : 0u);
	expect([&vec] { return vec.size(); }) == ((has_ran) ? 1u : 0u);
	expect([&vec](bool should_have_elements) -> bool { return vec.size() == ((should_have_elements) ? 1u : 0u); },
		   has_ran) == true;
	expect(&type::size, &vec) == ((has_ran) ? 1u : 0u);

	expect([&] { vec.size(); }) == nothrows();
	expect([&] { vec.at(-1u); }) == throws<std::range_error>();

	expect(
		[&vec](int value) {
			vec.push_back(value);
			return vec[vec.size() - 1];
		},
		5) == 5;
};

auto section_test = suite<"section">() = []() {
	section<"A">() = [] {
		section<"A">() = [] { section<"A">() = [] {}; };
		section<"B">() = [] {};
		section<"C">() = [] { section<"A">() = [] {}; };
	};

	section<"B">() = [] {};

	section<"C">() = [] { section<"A">() = [] {}; };

	/*
	AAA
	AB
	ACA
	B
	CA
	*/
};

// auto benchmark = benchmark<"queue">() = [] {
// 	section<"insertion">() = [] {

// 	};
// };
