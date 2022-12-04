#include <litmus/expect.hpp>

#include <algorithm>
#include <string>

#include <litmus/details/cache.hpp>
#include <litmus/details/exceptions.hpp>
#include <litmus/details/test_result.hpp>
#include <litmus/litmus.hpp>

#ifdef _MSC_VER
#define DEBUG_BREAK() __debugbreak()
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
	auto get_end_of_scope = [](const auto end, auto& it, auto offset, size_t depth = 0u,
							   char terminator = ')') -> size_t {
		char ignore = '0';
		for(; it != end; ++it)
		{
			std::string_view line = it->substr(offset);
			for(auto ch : line)
			{
				++offset;
				if(ignore != '0' && ignore == ch)
				{
					ignore = '0';
					continue;
				}
				if(ignore != '0' && (ch == '\'' || ch == '"'))
				{
					ignore = ch;
					continue;
				}

				if(ch == '(')
				{
					++depth;
				}
				if((ch == ')' || (ch == terminator && depth == 1)) && --depth == 0)
				{
					return offset - 1;
				}
			}
			offset = 0u;
		}
		return std::numeric_limits<size_t>::max();
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

	const auto& file = cache.get(config->source + source.file_name());

	auto it = std::next(file.begin(), source.line() - 1);

	// parse line for expect;
	auto expect = it->rfind(keyword, source.column());
	// we parse forward, potentially column isn't known
	if(expect == std::string::npos)
	{
		expect = it->find(keyword);
	}
	// if expect still isn't known, we are guaranteed to be within a scope, so we need to rfind every previous line
	// for a potential expect.
	while(expect == std::string::npos && --it != file.begin())
	{
		expect = it->rfind(keyword);
	}

	// todo: should be more robust. We should scan to previous line to make sure we aren't within a statement.
	except(expect == std::string::npos,
		   std::runtime_error("could not find the start of the lhs_user '" + std::string(keyword) + "' clause."));

	const auto lhs_user_begin	= expect + keyword.size();
	auto lhs_user_begin_ptr		= &((*it)[lhs_user_begin + 1]);
	const auto lhs_user_end		= get_end_of_scope(std::end(file), it, lhs_user_begin);
	const auto lhs_user_end_ptr = &((*it)[lhs_user_end]);

	while(*lhs_user_begin_ptr == '\r' || *lhs_user_begin_ptr == '\t' || *lhs_user_begin_ptr == '\n' ||
		  *lhs_user_begin_ptr == ' ')
	{
		++lhs_user_begin_ptr;
	}

	const auto lhs_ptr_diff = lhs_user_end_ptr - lhs_user_begin_ptr;
	except(lhs_ptr_diff < 0, std::runtime_error("lhs_user user clause has a detection error."));

	if(static_cast<size_t>(lhs_ptr_diff) > config->source_size_limit)
		lhs_user = "...";
	else
	{
		lhs_user = std::string{lhs_user_begin_ptr, lhs_user_end_ptr};
		lhs_user.erase(std::remove(lhs_user.begin(), lhs_user.end(), '\n'), lhs_user.end());
		lhs_user.erase(std::remove(lhs_user.begin(), lhs_user.end(), '\r'), lhs_user.end());
		lhs_user.erase(std::remove(lhs_user.begin(), lhs_user.end(), '\t'), lhs_user.end());
	}

	const auto& op_str	 = operation_to_string(operation);
	auto operation_begin = it->find(op_str, lhs_user_end + 1);
	while(operation_begin == std::string::npos && ++it != std::end(file))
	{
		operation_begin = it->find(op_str);
	}
	except(operation_begin == std::string::npos,
		   std::runtime_error("could not find the start of the operator '" + std::string(op_str) + "' clause."));

	const auto operation_end = operation_begin + op_str.size();
	auto rhs_user_begin		 = it->find_first_not_of(" \n\r\t", operation_end);
	while(rhs_user_begin == std::string::npos && ++it != std::end(file))
	{
		rhs_user_begin = it->find_first_not_of(" \n\r\t");
	}

	except(rhs_user_begin == std::string::npos, std::runtime_error("could not find the start of the rhs_user clause."));

	const auto rhs_user_begin_ptr = &((*it)[rhs_user_begin]);
	const auto rhs_user_end		  = get_end_of_scope(std::end(file), it, rhs_user_begin, 1u, ';');
	const auto rhs_user_end_ptr	  = &((*it)[rhs_user_end]);

	const auto rhs_ptr_diff = rhs_user_end_ptr - rhs_user_begin_ptr;
	except(rhs_ptr_diff < 0, std::runtime_error("rhs_user user clause has a detection error."));

	if(static_cast<size_t>(rhs_ptr_diff) > config->source_size_limit)
		rhs_user = "...";
	else
	{
		rhs_user = std::string{rhs_user_begin_ptr, rhs_user_end_ptr};
		rhs_user.erase(std::remove(rhs_user.begin(), rhs_user.end(), '\n'), rhs_user.end());
		rhs_user.erase(std::remove(rhs_user.begin(), rhs_user.end(), '\r'), rhs_user.end());
		rhs_user.erase(std::remove(rhs_user.begin(), rhs_user.end(), '\t'), rhs_user.end());

		// rhs_user.replace('\n');
	}
}
