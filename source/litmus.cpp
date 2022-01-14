#include <litmus/details/runner.hpp>
#include <litmus/details/context.hpp>
#include <litmus/litmus.hpp>
unsigned litmus::internal::runner_t::m_RefCount																  = 0;
std::unordered_map<const char*, litmus::internal::runner_t::test_t>* litmus::internal::runner_t::m_NamedTests = nullptr;
unsigned litmus::internal::config_t::m_RefCount																  = 0;
litmus::internal::config_t::data_t* litmus::internal::config_t::data										  = nullptr;
thread_local litmus::internal::suite_context_t litmus::internal::suite_context								  = {};

#include <exception>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <ostream>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <unordered_map>

#include <litmus/details/exceptions.hpp>
#include <litmus/details/test_result.hpp>


#include <litmus/formatter/detailed.hpp>
#include <litmus/formatter/json.hpp>
#include <litmus/formatter/compact.hpp>

std::string output_file = "";

using namespace litmus;

std::unique_ptr<formatter> default_formatter = std::make_unique<litmus::formatters::detailed_stream_formatter>();

class option
{
  public:
	option(std::string name, std::function<void(std::span<const std::string_view>)> reader, size_t min_options = 0,
		   size_t option_variance = 0)
		: m_Name(std::move(name)), m_Reader(std::move(reader)), m_MinTargets(min_options),
		  m_MaxTargets(m_MinTargets + option_variance + 1)
	{}
	auto read(std::span<const std::string_view> args) -> std::optional<size_t>
	{
		if(args.empty()) return std::nullopt;
		if(args[0].size() == m_Name.size() + 2 && args[0][0] == '-' && args[0][1] == '-' && args[0].substr(2) == m_Name)
		{
			size_t next = 1;
			if(m_MinTargets > 0 &&
			   internal::except(next == args.size(),
								std::runtime_error("incomplete argument, '" + m_Name + "' has no target")))
			{
				return std::nullopt;
			}

			if(m_MinTargets > 0 &&
			   internal::except(args.size() <= next || (!args[next].empty() && args[next][0] == '-') ||
									(args[next].size() > 1 && args[next][1] == '-'),
								std::runtime_error(
									"incomplete argument, target was invalid, values cannot start with leading '--'")))
			{
				return std::nullopt;
			}

			while(next < m_MaxTargets && args.size() > next &&
				  !((!args[next].empty() && args[next][0] == '-') || (args[next].size() > 1 && args[next][1] == '-')))
			{
				++next;
			}
			m_Reader(args.subspan(1, next - 1));

			return next;
		}
		return std::nullopt;
	}

  private:
	std::string m_Name;
	std::function<void(std::span<const std::string_view>)> m_Reader;
	size_t m_MinTargets, m_MaxTargets;
};

void configure(std::span<const std::string_view> args)
{
	std::vector<option> options{
		{"verbosity",
		 [](std::span<const std::string_view> args) {
			 if(args[0] == std::string_view("none"))
				 internal::config->verbosity = verbosity_t::NONE;
			 else if(args[0] == std::string_view("compact"))
				 internal::config->verbosity = verbosity_t::COMPACT;
			 else if(args[0] == std::string_view("normal"))
				 internal::config->verbosity = verbosity_t::NORMAL;
			 else if(args[0] == std::string_view("detailed"))
				 internal::config->verbosity = verbosity_t::DETAILED;
		 },
		 1},
		{"rerun-failed",
		 []([[maybe_unused]] std::span<const std::string_view> args) { internal::config->rerun_failed = true; }},
		{"single-threaded",
		 []([[maybe_unused]] std::span<const std::string_view> args) { internal::config->single_threaded = true; }},
		{"no-source",
		 []([[maybe_unused]] std::span<const std::string_view> args) { internal::config->no_source = true; }},
		{"source", [](std::span<const std::string_view> args) { internal::config->source = args[0]; }, 1, 0},
		{"output", [](std::span<const std::string_view> args) { output_file = args[0]; }, 1, 0},
		{"formatter",
		 [](std::span<const std::string_view> args) {
			 if(args[0] == "json")
			 {
				 default_formatter = std::make_unique<formatters::json>();
			 }
			 else if(args[0] == "detailed-plaintext")
			 {
				 default_formatter = std::make_unique<formatters::detailed_stream_formatter_no_color>();
			 }
			 else if(args[0] == "compact")
			 {
				 default_formatter = std::make_unique<formatters::compact>();
			 }
		 },
		 1, 0},
		{"source-size-limit",
		 [](std::span<const std::string_view> args) {
			 internal::config->source_size_limit = std::stoul(std::string(args[0]));
		 },
		 1, 0},
		{"category",
		 [](std::span<const std::string_view> args) {
			 internal::config->categories.insert(std::end(internal::config->categories), std::begin(args),
												 std::end(args));
		 },
		 1, std::numeric_limits<int>::max()}};

	size_t index{1};
	size_t control{1};
	while(std::any_of(std::begin(options), std::end(options),
					  [&](auto& option) {
						  if(index >= args.size()) return false;
						  if(auto res = option.read(args.subspan(index, args.size() - index)); res)
						  {
							  index += res.value();
							  return true;
						  }
						  return false;
					  }) &&
		  index < args.size() &&
		  !internal::except(control == index,
							std::runtime_error(std::string("unknown argument: ") + std::string(args[index]))))
	{
		control = index;
	}

#ifdef LITMUS_NO_SOURCE
	if(!config->no_source)
#else
	if(!config->no_source && strlen(source_location::current().file_name()) == 0)
#endif
	{
		config->no_source = true;
		std::cout << "turning off source expansion, binary was compiled with a compiler that has an incomplete "
					 "std::source_location implementation"
				  << std::endl;
	}

	if(!config->no_source)
	{
		std::ifstream stream(config->source + source_location::current().file_name());
		if(!stream.is_open())
		{
			throw std::runtime_error(
				"'--source' attribute is missing or incorrectly configured, please fix it or run the with the "
				"'--no-source' flag.");
		}
	}
	else
	{
		internal::config->source = {};
	}
}

void configure(int argc, char* argv[]) // NOLINT
{
	std::vector<std::string_view> args{};
	args.resize(argc);
	std::transform(&argv[0], &argv[argc], std::begin(args), [](char* str) -> std::string_view {
		return {str, strlen(str)};
	});
	configure({args});
}

auto litmus::run(int argc, char* argv[],
				 formatter* formatter) noexcept -> int // NOLINT
{
	configure(argc, argv);
	std::ofstream* filestream;
	if(formatter == nullptr)
	{
		formatter = default_formatter.get();
	}

	if(output_file.empty())
		formatter->set_stream(std::cout, true);
	else
	{
		filestream = new std::ofstream(output_file);
		formatter->set_stream(*filestream, false);
	}

	size_t fatal{0};
	size_t fail{0};
	size_t pass{0};
	std::chrono::microseconds duration{};

	auto real_start = std::chrono::high_resolution_clock::now();

	formatter->begin(internal::runner.size());

	struct suite_results_t
	{
		const char* name;
		size_t pass;
		size_t fail;
		size_t fatal;
		std::chrono::microseconds duration;
		source_location location;
		std::vector<std::pair<std::vector<std::string>, size_t>> templates{};
		std::vector<test_result_t> results{};
		bool skipped;
	};

	auto run_suite = [](const char* name, const runner_t::test_t& test_units,
						std::span<std::string> categories = std::span<std::string>{}) -> suite_results_t {
		suite_results_t result{};
		result.name = name;
		if(test_units.empty()) return {};
		size_t local_fatal{0};
		size_t local_fail{0};
		size_t local_pass{0};
		std::chrono::microseconds local_duration{};

		std::chrono::microseconds suite_duration{};
		for(const auto& [uid, tests] : test_units)
		{
			result.templates.emplace_back(tests.templates, tests.functions.size());
			for(const auto& test : tests.functions)
			{
				auto res = test();
				if(res.results.empty()) continue;
				result.results.emplace_back(std::move(res));

				result.results.back().get_result_values(local_pass, local_fail, local_fatal, local_duration);
				result.pass += local_pass;
				result.fail += local_fail;
				result.fatal += local_fatal;
				result.duration += local_duration;
			}
		}
		result.skipped = result.results.empty();
		if(!result.skipped)
			result.location = std::begin(result.results)->root().location;
		return result;
	};

	auto format_suite = [&formatter](const suite_results_t& suite) {
		formatter->suite_begin(suite.name, suite.pass, suite.fail, suite.fatal, suite.location, suite.duration);
		auto result = std::begin(suite.results);
		for(const auto& [templates, tests_size] : suite.templates)
		{
			if(!templates.empty()) formatter->suite_iterate_templates(templates);
			for(auto i = 0u; i < tests_size; ++i)
			{
				formatter->suite_iterate(templates, result->root().parameters);
				result->to_string(formatter);
				result = std::next(result);
			}
		}
		formatter->suite_end(suite.name, suite.pass, suite.fail, suite.fatal, suite.location, suite.duration);
	};

	if(config->single_threaded)
	{
		for(const auto& [name, test_units] : internal::runner)
		{
			auto suite = run_suite(name, test_units);
			if(suite.skipped) continue;
			pass += suite.pass;
			fail += suite.fail;
			fatal += suite.fatal;
			duration += suite.duration;
			format_suite(suite);
		}
	}
	else
	{
		std::vector<std::future<suite_results_t>> suite_results{};
		suite_results.reserve(internal::runner.size());
		for(const auto& [name, test_units] : internal::runner)
		{
			suite_results.emplace_back(std::async(std::launch::async, run_suite, name, test_units,
												  std::span<std::string>{config->categories}));
		}

		for(auto& suite_future : suite_results)
		{
			auto suite = suite_future.get();
			if(suite.skipped) continue;
			pass += suite.pass;
			fail += suite.fail;
			fatal += suite.fatal;
			duration += suite.duration;
			format_suite(suite);
		}
	}

	formatter->write_totals(
		pass, fail, fatal, duration,
		std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - real_start));
	formatter->flush();
	return (fail > 0 || fatal > 0) ? 1 : 0;
}
