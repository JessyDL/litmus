# Litmus

Disclaimer: currently this repository is still under active development. Documentation, examples, and API stability will be achieved after the first versioned release which *should be* soon.

## About
Litmus is a unit testing framework that avoids the traditional macro heavy design in favour of functional style unit tests. It supports fixtures, templates, and generators out of the box.

Its primary focus is on facilitating functional style tests, comes with template/NTTP generators out of the box allowing you to trivially test heavily templatized classes and functions.

Besides that it has some support for expanding your user code into the test output, so you can see the context of the test itself. See more about that in // todo

As this is a side project the features are constrained to what my personal usage needs are. Feel free to request features or contribute your own through a PR.

## Setup

There are a couple of ways to setup `litmus`, depending on the amount of control you wish to take.

The "auto" setup, which does everything for you
```cpp
#define LITMUS_FULL 			// includes all headers and defines a `main()` function for you.
#include <litmus/litmus.hpp>
```

If you want a bit more control, you can instead use the `LITMUS_MAIN()` macro after including `<litmus/litmus.hpp>` to control where it goes.

If you want a custom main, you can use the following example as a template for how to setup your `main()`
```cpp
int main(int argc, char* argv[])
{
	return litmus::run(argc, argv);
}
```

You'll need to provide the `--source` parameter when launching, this is either an absolute, or relative path in relation to the executable, to the location where the tests' source code resides. If you don't want source expansion, or don't have access to the source, launch using `--no-source`.

## Documentation
### Suite
Suites are the top level testing unit, they are meant to be independent work tasks that can potentially run in parallel. You can instantiate a testing `suite` by includeing `<litmus/suite.hpp>`.

*info: By default every suite's permutation will create an async task. Launch using `--single-threaded` if you want to disable multithreaded testing.*

The makeup of the function looks as follows:
```cpp
auto test_obj = suite<"name", "category0", ..., "categoryN">(parameters...) = [functional object];
```

Suites can have a name, and 0 to N categories. The name is a *non-unique mandatory* string used for the output of the tests, while the categories can be used to restrict which types of tests to run. Note that the name can act as a "category of one" meaning you can limit the execution based on the name as well.

You can pass any form of parameters to the suite, these will be copied to the suite when run (and as such the parameters require copy-ability). This is also your fixture functionality on the `suite` scope.
Generators can also be passed, and depending on the generator its side effects will either pass extra parameters into the function, or pass template arguments. See the generator section for more info.

A suite can be invoked many times during a single run. This is controlled by adding permutations, either through adding `section` scopes (see next topic), or by adding a `generator`. Following is an example of a suite that will be invoked 10 times with an incrementing number from [0-9].

```cpp
// will invoke the suite 10 times from [0, 9]
auto test = suite<"vector">(generator::range<size_t, 0, 9>{}) = [](size_t count) {
	std::vector<int> vec{};
	vec.resize(count);
	expect(vec.size()) == count;
};
```

### Section
Sections are the next level of scope control available. They allow for divergent behaviour based on "common" functionality in upper scopes. Suites will be invoked as many times as there exists unique permutations of sections. See the following example showcasing scope based permutations:

```cpp
auto section_test = test<"vector">() = []<typename T> {
	std::vector<T> vec{};
	section<"insert">() = [] {
		section<"erase">() = []{};
		section<"clear">() = []{};
	};
	section<"resize">() = []{};
};
```
This will output 3 unique sections in a test suite, `[insert.erase]`, `[insert.clear]`, and `[resize]` in that order. Note that the sections not mentioned in the runs name are not ran at all, as example the `clear` section will never see the side-effects of the `erase` section.

*Info: scope based permutation is not the only type of permutation available, aside from those you have parameter based permutation through generators, and typename/NTTP based permutation. See the generator section for more info.*

This can be handy when you wish to set up some common data (such as a prefilled vector), before submitting it to a battery of tests such as resizing, inserting, etc...

### Expect
Expect is the actual test unit of the library. They wrap the value, potentially evaluating the target if it satisfies the concept of invocable, and then uses the comparison operators to verify the result. Following is an example of several comparisons within a block.

```cpp
auto simple_example = suite<"expect">() = []{
	int value{5};
	expect(value) == 5;
	expect(value) < 6;
	expect(value) != -1;
	expect([]{ return true; }) != false;
};
```

#### ***Exceptions***
Exception testing is supported using the `throws_t<Exceptions...>` and `nothrows_t`. Following is an example that showcases both.

```cpp
auto exception_test = suite<"exceptions">() = []{
	std::vector<int> vec{};
	// if function doesn't throw, the expect returns true, otherwise false
	expect(nothrows_t{}(&vec::size, vec)) == true;
	// if function does throw, returns true (otherwise false)
	expect(throws_t<>{}(&vec::resize, size_t{-1u})) == true;
};
```

Note that `throws_t<>` without typename arguments is equivalent to "check if any exception is thrown". Insert exception types in the list to test the existence of specific exception types.


## Examples

All examples implicitly use `using namespace litmus;` for brevity reasons. It's up to you how to structure your own code.

Basic example

The most easiest, and recommended method is defining a variable somewhere in an implementation file. 
```cpp
auto basic_test = suite<"basic">() = []() {
	expect(5) == 5;
};
```

## License
**MIT License**
Please refer to the `LICENSE` file for further info.