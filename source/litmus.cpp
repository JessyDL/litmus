#include <litmus/details/runner.hpp>
#include <litmus/details/context.hpp>
#include <litmus/litmus.hpp>
unsigned litmus::internal::runner_t::m_RefCount																  = 0;
std::vector<std::function<litmus::internal::test_result_t()>>* litmus::internal::runner_t::m_Tests			  = nullptr;
std::unordered_map<const char*, litmus::internal::runner_t::test_t>* litmus::internal::runner_t::m_NamedTests = nullptr;
unsigned litmus::internal::config_t::m_RefCount																  = 0;
litmus::internal::config_t::data_t* litmus::internal::config_t::data										  = nullptr;
thread_local litmus::internal::suite_context_t litmus::internal::suite_context								  = {};

#include <cstring>
#include <chrono>
#include <exception>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <unordered_map>

#include <litmus/details/console_formatting.hpp>
#include <litmus/details/exceptions.hpp>
#include <litmus/details/source_location.hpp>
#include <litmus/details/test_result.hpp>
#include <litmus/details/utility.hpp>
#include <litmus/details/verbosity.hpp>

using namespace litmus;

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

	if(!config->no_source && strlen(source_location::current().file_name()) == 0)
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


// auto litmus::run(int argc, char* argv[], std::ostream& stream) noexcept -> int // NOLINT
#include <litmus/formatter.hpp>
class stream_formatter final : public litmus::formatter
{
  public:
	stream_formatter(const out_wrapper_t& stream) noexcept : stream(stream) {}
	void scope_begin(const test_result_t::scope_t& scope) override
	{
		const size_t style_index = (scope.fatal > 0) ? 2 : (scope.fail > 0) ? 1 : (scope.pass > 0) ? 0 : 3;
		auto pass_str			 = std::to_string(scope.pass);
		auto total_str			 = ((scope.fatal > 0) ? "?" : std::to_string(scope.fail + scope.fatal + scope.pass));

		std::string parameters{};
		size_t param_size{0u};
		if(!scope.parameters.empty())
		{
			parameters = std::move(join(scope.parameters, ", "));
			param_size += parameters.size() + 5;
			parameters = dim(italics(combine_text(" [ ", std::move(parameters), " ]")));
		}

		auto duration_str = std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(scope.duration_end -
																								 scope.duration_start)
											   .count()) +
							"μs ";

		auto lhs = combine_text(std::string((scope.id.size() + extra_depth) * 2, ' '), bold(scope.name),
								std::move(parameters));
		auto rhs =
			duration_str + colour(combine_text('[', pass_str, '/', total_str, ']'), style_colours[style_index][0],
								  style_colours[style_index][1], style_colours[style_index][2]);
		auto padding		= (scope.id.size() + extra_depth) * 2 + scope.name.size() + param_size;
		const auto rhs_size = pass_str.size() + total_str.size() + 2 + duration_str.size();
		padding				= (padding > (120 - rhs_size)) ? 0u : 120 - rhs_size - padding;
		stream.output(combine_text(std::move(lhs), std::string(padding, ' '), std::move(rhs), '\n'));
		stream.output(colour(combine_text(std::string((scope.id.size() + extra_depth) * 2, ' '),
										  std::string(120u - (scope.id.size() + extra_depth) * 2, '-'), '\n'),
							 "80", "80", "96"));
	}

	void expect(const test_result_t::expect_t& expect, const test_result_t::scope_t& scope) override
	{
		using result_t			   = test_result_t::expect_t::result_t;
		using operation_t		   = test_result_t::expect_t::operation_t;
		const size_t outcome_index = (expect.result == result_t::fatal) ? 2 : (expect.result == result_t::fail) ? 1 : 0;


		if(!expect.info.empty())
		{
			stream.output(combine_text(std::string((scope.id.size() + 1u) * 2u + 8u, ' '),
									   colour(italics(expect.info), "0", "139", "139"), '\n'));
		}

		auto codeblock = [](const auto& expect) -> std::string {
			auto value_block = [](const auto& lhs, const auto& rhs, const auto& operation) {
				return combine_text("[ ", lhs, ' ', operation, ' ', rhs, " ]");
			};

			auto user_block = [](const auto& lhs, const auto& rhs, const auto& operation, const auto& lhs_user,
								 const auto& rhs_user) -> std::string {
				if((lhs == lhs_user && rhs == rhs_user) || lhs.empty() || (lhs.empty() && rhs.empty())) return {};

				if(rhs == rhs_user)
				{
					return combine_text(italics("lhs [ "), dim(lhs), italics(" ]"));
				}
				return combine_text(italics("[ lhs [ "), dim(lhs), italics(" ] "), italics(std::string(operation)),
									italics(" rhs [ "), dim(rhs), italics(" ] ]"));
			};

			std::string_view operation{};
			switch(expect.operation)
			{
			case operation_t::equal:
				operation = "==";
				break;
			case operation_t::inequal:
				operation = "!=";
				break;
			case operation_t::less_equal:
				operation = "<=";
				break;
			case operation_t::greater_equal:
				operation = ">=";
				break;
			case operation_t::less_than:
				operation = "<";
				break;
			case operation_t::greater_than:
				operation = ">";
				break;
			}

			return combine_text(
				value_block(expect.lhs_value, expect.rhs_value, operation), ' ',
				user_block(expect.lhs_user, expect.rhs_user, operation, expect.lhs_value, expect.rhs_value));

			// return combine_text(condense_block(expect., expect.lhs_user), ' ', bold(std::string(operation)),
			//					' ', condense_block(expect., expect.));
		}(expect);
		stream.output(combine_text(
			std::string((scope.id.size() + 1 + extra_depth) * 2, ' '),
			colour(combine_text(outcome[outcome_index],
								((outcome_index == 2) ? std::string_view("=> ") : std::string_view(" => "))),
				   style_colours[outcome_index][0], style_colours[outcome_index][1], style_colours[outcome_index][2]),
			codeblock, '\n'));
	}

	void suite_begin(const char* name, size_t pass, size_t fail, size_t fatal,
					 [[maybe_unused]] const source_location& location, std::chrono::microseconds duration) override
	{
		auto name_str	 = std::string(name);
		auto pass_str	 = std::to_string(pass);
		auto total_str	= (fatal > 0) ? std::string("?") : std::to_string(pass + fail);
		auto duration_str = std::to_string(duration.count()) + "μs";

		auto rhs_size = pass_str.size() + total_str.size() + duration_str.size() + 3;

		size_t outcome_index = (fatal > 0) ? 2 : (fail > 0) ? 1 : (pass > 0) ? 0 : 3;
		std::string rhs		 = combine_text(
			 duration_str, colour(combine_text(" [", pass_str, "/", total_str, "]"), style_colours[outcome_index][0],
								  style_colours[outcome_index][1], style_colours[outcome_index][2]));


		stream.output(combine_text(bold(name_str), std::string(120u - name_str.size() - rhs_size, ' '), rhs, '\n'));
		extra_depth   = 0u;
		has_templates = false;

		stream.output(colour(combine_text(std::string(120u, '-'), '\n'), "80", "80", "96"));
	}

	void suite_end([[maybe_unused]] const char* name, [[maybe_unused]] size_t pass, size_t fail, size_t fatal,
				   const source_location& location, [[maybe_unused]] std::chrono::microseconds duration) override
	{
		auto filename = config->source + location.file_name() + ":" + std::to_string(location.line());

		if(fail != 0 || fatal != 0)
		{
			stream.output(colour(combine_text("  ", std::to_string(fail), " fails and ", std::to_string(fatal),
											  " fatals in ", filename, '\n'),
								 "255", "0", "0"));
		}
		stream.output("\n");
	}

	void suite_iterate_templates(std::string_view templates) override
	{
		stream.output(combine_text("  template<", templates, ">\n"));
		extra_depth   = 1u;
		has_templates = true;
	}
	void suite_iterate_parameters(const std::vector<std::string>& parameters) override
	{
		std::string pstr{};
		extra_depth = (has_templates) ? 2u : 1u;

		pstr = std::move(join(parameters, ", "));
		pstr = dim(italics(combine_text(std::string(extra_depth * 2u, ' '), "arguments { ", std::move(pstr), " }\n")));

		stream.output(pstr);
	}


	void write_totals(size_t pass, size_t fail, size_t fatal, std::chrono::microseconds duration) override
	{
		if(fail == 0 && fatal == 0)
		{
			stream.output(colour(
				bold(combine_text(
					"\nlitmus detected all ", std::to_string(pass), " tests passed in ",
					std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() * 0.001),
					"s.\n")),
				"0", "255", "0"));
		}
		else
		{
			stream.output(colour(
				bold(combine_text(
					"\nlitmus detected ", std::to_string(fail), " fails and ", std::to_string(fatal), " fatals in ",
					std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() * 0.001),
					"s.\n")),
				"255", "0", "0"));
		}
	}

	constexpr static std::array<std::string_view, 4> outcome{"PASS", "FAIL", "FATAL", "ERROR"};

	constexpr static std::array<std::array<const char*, 3>, 4> style_colours{
		{{"0", "180", "50"}, {"220", "0", "0"}, {"200", "0", "200"}, {"200", "150", "50"}}};
	const out_wrapper_t& stream;

	size_t extra_depth{0u};
	bool has_templates{false};
};

template <typename R>
auto is_ready(std::future<R> const& f) -> bool
{
	return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

auto litmus::run(int argc, char* argv[], out_wrapper_t stream, formatter* formatter) noexcept -> int // NOLINT
{
	configure(argc, argv);

	bool local_formatter = formatter == nullptr;
	if(local_formatter)
	{
		formatter = new stream_formatter(stream);
	}

	size_t fatal{0};
	size_t fail{0};
	size_t pass{0};
	std::chrono::microseconds duration{};
	config->single_threaded = true;
	if(config->single_threaded)
	{
		for(const auto& [name, test_units] : internal::runner)
		{
			std::vector<test_result_t> results{};

			size_t suite_fatal{0};
			size_t suite_fail{0};
			size_t suite_pass{0};
			std::chrono::microseconds suite_duration{};
			for(const auto& [templates, tests] : test_units)
			{
				for(const auto& test : tests)
				{
					results.emplace_back(test());
					size_t local_fatal{0};
					size_t local_fail{0};
					size_t local_pass{0};
					std::chrono::microseconds local_duration{};
					results.back().get_result_values(local_pass, local_fail, local_fatal, local_duration);
					suite_pass += local_pass;
					suite_fail += local_fail;
					suite_fatal += local_fatal;
					suite_duration += local_duration;
				}
			}

			pass += suite_pass;
			fail += suite_fail;
			fatal += suite_fatal;
			duration += suite_duration;
			auto result		 = std::begin(results);
			const auto& root = result->root();

			formatter->suite_begin(name, suite_pass, suite_fail, suite_fatal, root.location, suite_duration);
			for(const auto& [templates, tests] : test_units)
			{
				if(!templates.empty()) formatter->suite_iterate_templates(templates);
				for(auto i = 0u; i < tests.size(); ++i)
				{
					result->to_string(formatter);
					result = std::next(result);
				}
			}
			formatter->suite_end(name, suite_pass, suite_fail, suite_fatal, root.location, suite_duration);
		}
	}
	else
	{
		// std::vector<std::future<internal::test_result_t>> results{};
		// results.reserve(internal::runner_t::size());
		// for(const auto& test : internal::runner)
		// {
		// 	results.emplace_back(std::async(test));
		// }

		// while(!results.empty())
		// {
		// 	for(auto it = std::begin(results); it != std::end(results); ++it)
		// 	{
		// 		if(is_ready(*it))
		// 		{
		// 			const auto& result = it->get();
		// 			size_t local_fatal{0};
		// 			size_t local_fail{0};
		// 			size_t local_pass{0};
		// 			std::chrono::microseconds local_duration{};
		// 			result.get_result_values(local_pass, local_fail, local_fatal, local_duration);
		// 			pass += local_pass;
		// 			fail += local_fail;
		// 			fatal += local_fatal;
		// 			duration += local_duration;
		// 			result.to_string(formatter);
		// 			results.erase(it);
		// 			break;
		// 		}
		// 	}
		// }
	}

	formatter->write_totals(pass, fail, fatal, duration);
	if(local_formatter)
	{
		delete(formatter);
	}

	return (fail > 0 || fatal > 0) ? 1 : 0;
}
