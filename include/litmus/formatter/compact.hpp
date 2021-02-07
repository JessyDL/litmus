#pragma once
#include <litmus/formatter.hpp>

namespace litmus::formatters
{
	class compact final : public litmus::formatter
	{
	  public:
		void scope_begin(const test_result_t::scope_t& scope) override
		{
			if(!log_suite) return;

			log_scope = scope.fatal > 0 || scope.fail > 0;
			if(!log_scope) return;
			const size_t style_index = (scope.fatal > 0) ? 2 : (scope.fail > 0) ? 1 : (scope.pass > 0) ? 0 : 3;
			auto pass_str			 = std::to_string(scope.pass);
			auto total_str = ((scope.fatal > 0) ? "?" : std::to_string(scope.fail + scope.fatal + scope.pass));

			std::string parameters{};
			size_t param_size{0u};
			if(!scope.parameters.empty())
			{
				parameters = std::move(join(scope.parameters, ", "));
				param_size += parameters.size() + 5;
				parameters = dim(italics(combine_text(" [ ", std::move(parameters), " ]")));
			}

			auto duration_str = std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(
												   scope.duration_end - scope.duration_start)
												   .count()) +
								"μs ";

			auto lhs = combine_text(std::string((scope.id.size() + extra_depth) * 2, ' '), bold(scope.name),
									std::move(parameters));
			auto rhs =
				duration_str + colour(combine_text('[', pass_str, '/', total_str, ']'), style_colours[style_index]);
			auto padding		= (scope.id.size() + extra_depth) * 2 + scope.name.size() + param_size;
			const auto rhs_size = pass_str.size() + total_str.size() + 2 + duration_str.size();
			padding				= (padding > (120 - rhs_size)) ? 0u : 120 - rhs_size - padding;
			output() << (combine_text(std::move(lhs), std::string(padding, ' '), std::move(rhs), '\n'));
			output() << (colour(combine_text(std::string((scope.id.size() + extra_depth) * 2, ' '),
											 std::string(120u - (scope.id.size() + extra_depth) * 2, '-'), '\n'),
								80, 80, 96));
		}

		void scope_end([[maybe_unused]] const test_result_t::scope_t& scope) override { log_scope = false; }

		void expect(const test_result_t::expect_t& expect, const test_result_t::scope_t& scope) override
		{
			if(!log_scope) return;
			using result_t	= test_result_t::expect_t::result_t;
			using operation_t = test_result_t::expect_t::operation_t;
			const size_t outcome_index =
				(expect.result == result_t::fatal) ? 2 : (expect.result == result_t::fail) ? 1 : 0;


			if(!expect.info.empty())
			{
				output() << (combine_text(std::string((scope.id.size() + 1u + extra_depth) * 2u + 8u, ' '),
										  colour(italics(expect.info), 0, 139, 139), '\n'));
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
			output() << (combine_text(
				std::string((scope.id.size() + 1 + extra_depth) * 2, ' '),
				colour(combine_text(outcome[outcome_index],
									((outcome_index == 2) ? std::string_view("=> ") : std::string_view(" => "))),
					   style_colours[outcome_index]),
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
				 duration_str, colour(combine_text(" [", pass_str, "/", total_str, "]"), style_colours[outcome_index]));


			output() << (combine_text(bold(name_str), std::string(120u - name_str.size() - rhs_size, ' '), rhs, '\n'));
			extra_depth   = 0u;
			has_templates = false;

			output() << (colour(combine_text(std::string(120u, '-'), '\n'), 80, 80, 96));

			log_suite = fail > 0 || fatal > 0;
		}

		void suite_end([[maybe_unused]] const char* name, [[maybe_unused]] size_t pass, size_t fail, size_t fatal,
					   const source_location& location, [[maybe_unused]] std::chrono::microseconds duration) override
		{
			auto filename = config->source + location.file_name() + ":" + std::to_string(location.line());

			if(fail != 0 || fatal != 0)
			{
				output() << (colour(combine_text("  ", std::to_string(fail), " fails and ", std::to_string(fatal),
												 " fatals in ", filename, '\n'),
									255, 0, 0));
			}
			if(log_suite) output() << ("\n");
			log_suite = false;
		}

		void suite_iterate_templates(const std::vector<std::string>& templates) override
		{
			if(!log_suite) return;
			std::string pstr{};
			pstr = std::move(join(templates, ", "));
			output() << (combine_text(colour("  template<", 30, 180, 255), pstr, colour(">\n", 30, 180, 255)));
			extra_depth   = 1u;
			has_templates = true;
		}
		void suite_iterate_parameters(const std::vector<std::string>& parameters) override
		{
			if(!log_suite) return;
			std::string pstr{};
			extra_depth = (has_templates) ? 2u : 1u;

			pstr = std::move(join(parameters, ", "));
			pstr = italics(combine_text(std::string(extra_depth * 2u, ' '), colour("arguments { ", 249, 242, 99),
										std::move(pstr), colour(" }\n", 249, 242, 99)));

			output() << (pstr);
		}

		std::string time_to_string(std::chrono::microseconds duration)
		{
			std::string time{};
			using namespace std::literals::chrono_literals;
			bool in_mins = duration > 1min;
			if(in_mins)
			{
				auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration);
				time		 = std::to_string(minutes.count());
				duration -= minutes;
				time += ":";
			}
			bool in_seconds = duration > 1s;
			if(in_seconds)
			{
				auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
				time		 = std::to_string(seconds.count());
				duration -= seconds;
				time += ":";
			}
			time += std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
			time += (in_mins) ? "m" : (in_seconds) ? "s" : "ms";
			return time;
		}


		void write_totals(size_t pass, size_t fail, size_t fatal, std::chrono::microseconds duration,
						  std::chrono::microseconds user_duration) override
		{
			std::array<uint8_t, 3> colour_value =
				((fail == 0 && fatal == 0) ? std::array<uint8_t, 3>{0, 255, 0} : std::array<uint8_t, 3>{255, 0, 0});
			if(fail == 0 && fatal == 0)
			{
				output() << (colour(
					bold(combine_text("\nlitmus detected all ", std::to_string(pass), " tests passed in ")),
					colour_value));
			}
			else
			{
				output() << (colour(bold(combine_text("\nlitmus detected ", std::to_string(fail), " fails and ",
													  std::to_string(fatal), " fatals in ")),
									colour_value));
			}


			output() << colour(
				bold(combine_text(time_to_string(duration), " (real ", time_to_string(user_duration), ").\n")),
				colour_value);
		}

		constexpr static std::array<std::string_view, 4> outcome{"PASS", "FAIL", "FATAL", "ERROR"};

		constexpr static std::array<std::array<uint8_t, 3>, 4> style_colours{
			{{0, 180, 50}, {220, 0, 0}, {200, 0, 200}, {200, 150, 50}}};

		size_t extra_depth{0u};
		bool has_templates{false};
		bool log_suite{false};
		bool log_scope{false};
	};
} // namespace litmus::formatters
