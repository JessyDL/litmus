#include <litmus/expect.hpp>

#include <algorithm>
#include <string>

#include <litmus/details/cache.hpp>
#include <litmus/details/exceptions.hpp>
#include <litmus/details/test_result.hpp>
#include <litmus/litmus.hpp>

#ifdef _MSC_VER
#define DEBUG_BREAK() __debugbreak()
#elif __APPLE__
#define DEBUG_BREAK() __builtin_trap()
#else
#include <signal.h>
#define DEBUG_BREAK() raise(SIGTRAP)
#endif

void litmus::internal::trigger_break(bool res, bool is_fatal) noexcept
{
	if(!res && (config->break_on_fail || (config->break_on_fatal && is_fatal)))
	{
		DEBUG_BREAK();
	}
}

void litmus::internal::evaluate(const source_location& source, test_result_t::expect_t::operation_t operation,
								std::string_view keyword, std::string& lhs_user, std::string& rhs_user)
{
	if(config->no_source) return;
	auto get_end_of_scope = [](std::string_view source, size_t depth = 1u, char open_scope = '(',
							   char close_scope = ')', char terminator = ')') -> size_t {
		std::string_view ignore_chars{"\"'"};
		char ignore = '0';
		size_t result{0};
		for(auto ch : source)
		{
			++result;
			if(ignore != '0')
			{
				if(ignore == ch)
				{
					ignore = '0';
				}
				continue;
			}
			else if(ignore_chars.find(ch) != ignore_chars.npos)
			{
				ignore = ch;
				continue;
			}

			if(ch == open_scope)
			{
				++depth;
			}
			else if(ch == close_scope)
			{
				--depth;
				if(depth == 0 && close_scope == terminator)
				{
					return result - 1;
				}
			}
			else if(depth == 0 && ch == terminator)
			{
				return result - 1;
			}
		}
		return source.npos;
	};

	auto operation_to_string = [](auto operation) -> std::string_view {
		using operation_t = test_result_t::expect_t::operation_t;

		switch(operation)
		{
		case operation_t::equal:
			return "==";
		case operation_t::inequal:
			return "!=";
		case operation_t::less_equal:
			return "<=";
		case operation_t::less_than:
			return "<";
		case operation_t::greater_equal:
			return ">=";
		case operation_t::greater_than:
			return ">";
		}
		throw std::runtime_error("unhandled operation");
	};

	constexpr auto blank_space = std::string_view{"\r\n\t "};
	const auto& file		   = cache.get(config->source + source.file_name());

	auto source_view = file.substr(source.line() - 1);

	// parse line for expect;
	size_t lhs_begin_scope{0};
	while(lhs_begin_scope != std::string::npos)
	{
		lhs_begin_scope = source_view.find(keyword, lhs_begin_scope);
		if(lhs_begin_scope != std::string::npos)
		{
			if(auto next = source_view.find_first_not_of(blank_space, lhs_begin_scope + keyword.size());
			   next != std::string::npos && source_view[next] == '(')
			{
				lhs_begin_scope = next + 1;
				break;
			}
		}
	}

	except(lhs_begin_scope == std::string::npos,
		   std::runtime_error("could not find the start of the lhs_user '" + std::string(keyword) + "' clause."));

	lhs_begin_scope = source_view.find_first_not_of(blank_space, lhs_begin_scope);
	except(lhs_begin_scope == std::string::npos,
		   std::runtime_error("could not find the start of the lhs_user clause."));
	auto lhs_size = get_end_of_scope(source_view.substr(lhs_begin_scope), 1, '(', ')', ')');
	except(lhs_size == std::string::npos, std::runtime_error("could not find the end of the lhs_user clause."));

	if(lhs_size > config->source_size_limit)
	{
		lhs_user.reserve(config->source_size_limit);
		lhs_user = source_view.substr(lhs_begin_scope, config->source_size_limit - 3);
		lhs_user += std::string_view{"..."};
	}
	else
	{
		lhs_user = source_view.substr(lhs_begin_scope, lhs_size);
	}
	lhs_user.erase(std::remove(lhs_user.begin(), lhs_user.end(), '\n'), lhs_user.end());
	lhs_user.erase(std::remove(lhs_user.begin(), lhs_user.end(), '\r'), lhs_user.end());
	lhs_user.erase(std::remove(lhs_user.begin(), lhs_user.end(), '\t'), lhs_user.end());

	auto op_view = file.substr(source.line() - 1, lhs_begin_scope + lhs_size + 1);

	const auto& op_str	 = operation_to_string(operation);
	auto operation_begin = op_view.find(op_str);
	except(operation_begin == std::string::npos,
		   std::runtime_error("could not find the start of the operator '" + std::string(op_str) + "' clause."));

	const auto operation_end = lhs_begin_scope + lhs_size + 1 + operation_begin + op_str.size();
	auto rhs_user_view		 = source_view.substr(operation_end);
	rhs_user_view			 = rhs_user_view.substr(rhs_user_view.find_first_not_of(blank_space));

	except(rhs_user_view.empty(), std::runtime_error("could not find the start of the rhs_user clause."));

	rhs_user_view = rhs_user_view.substr(0, get_end_of_scope(rhs_user_view, 0, '(', ')', ';'));

	if((rhs_user_view.size() > config->source_size_limit))
	{
		rhs_user.reserve(config->source_size_limit);
		rhs_user = rhs_user_view.substr(0, config->source_size_limit - 3);
		rhs_user += std::string_view{"..."};
	}
	else
	{
		rhs_user = rhs_user_view;
	}
	rhs_user.erase(std::remove(rhs_user.begin(), rhs_user.end(), '\n'), rhs_user.end());
	rhs_user.erase(std::remove(rhs_user.begin(), rhs_user.end(), '\r'), rhs_user.end());
	rhs_user.erase(std::remove(rhs_user.begin(), rhs_user.end(), '\t'), rhs_user.end());
}
