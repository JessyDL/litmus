#pragma once
#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <numeric>
#include <span>
#include <stack>
#include <string>
#include <variant>
#include <vector>

#include <litmus/details/source_location.hpp>
#include <litmus/details/verbosity.hpp>
#include <litmus/litmus.hpp>

#ifndef LITMUS_MAX_DEPTH
#define LITMUS_MAX_DEPTH 32
#endif

#ifndef LITMUS_MAX_TEST_ID_TYPE
#define LITMUS_MAX_TEST_ID_TYPE uint8_t
#endif

namespace litmus
{
	inline namespace internal
	{
		struct test_id_t
		{
			[[nodiscard]] auto as_string() const noexcept -> std::string_view
			{
				return {reinterpret_cast<const char*>(m_Data.data()),
						(sizeof(LITMUS_MAX_TEST_ID_TYPE) / sizeof(char)) * LITMUS_MAX_DEPTH};
			}

			[[nodiscard]] auto to_string() const noexcept -> std::string
			{
				const auto c_sz = size();
				if(c_sz == 0) return {};
				std::string res{};
				res.reserve(c_sz * 2 - 1);
				for(auto i = 0u; i < c_sz - 1; ++i)
				{
					res.append(std::to_string(m_Data[i]));
					res += '-';
				}
				res.append(std::to_string(m_Data[c_sz - 1]));
				return res;
			}

			void from_string(std::string_view str) // NOLINT(readability-convert-member-functions-to-static)
			{
				if(str.size() <= (sizeof(LITMUS_MAX_TEST_ID_TYPE) / sizeof(char)) * LITMUS_MAX_DEPTH)
				{
					std::memcpy(m_Data.data(), str.data(), sizeof(LITMUS_MAX_TEST_ID_TYPE) * str.size());
				}
			}

			[[nodiscard]] auto size() const noexcept -> size_t { return m_Size; }
			[[nodiscard]] auto byte_size() const noexcept -> size_t { return sizeof(LITMUS_MAX_TEST_ID_TYPE) * m_Size; }

			[[nodiscard]] auto empty() const noexcept -> bool { return m_Size == 0; }

			void clear() noexcept { m_Size = 0; }

			void set(size_t index, LITMUS_MAX_TEST_ID_TYPE value) noexcept
			{
				m_Data[index] = value;
				m_Size		  = std::max(m_Size, index + 1);
			}

			void resize(size_t index) noexcept { m_Size = index; }

			[[nodiscard]] auto get(size_t index) const noexcept -> LITMUS_MAX_TEST_ID_TYPE { return m_Data[index]; }


		  private:
			size_t m_Size{0};
			std::array<LITMUS_MAX_TEST_ID_TYPE, LITMUS_MAX_DEPTH> m_Data{};
		};

		struct test_result_t
		{
			struct scope_t
			{
				std::string name{};
				std::vector<std::string> parameters{};
				test_id_t id{};
				const source_location& location;
				size_t pass{0};
				size_t fail{0};
				size_t fatal{0};
				size_t children{0};
				std::chrono::time_point<std::chrono::high_resolution_clock> duration_start{};
				std::chrono::time_point<std::chrono::high_resolution_clock> duration_end{};
			};

			struct scope_close_t
			{
				size_t scope_index{};
			};

			struct expect_t
			{
				std::string lhs_value{};
				std::string rhs_value{};
				std::string lhs_user{};
				std::string rhs_user{};

				enum class operation_t
				{
					equal,		   // ==
					inequal,	   // !=
					greater_equal, // >=
					less_equal,	   // <=
					less_than,	   // <
					greater_than,  // >
				} operation{};
				std::string info{};
				enum class result_t
				{
					pass,
					fail,
					fatal,
				} result{};

				size_t parent_index{};
			};

			void scope_open(const std::string& name, test_id_t id, const source_location& location,
							std::vector<std::string> parameters = {})
			{
				active_scope_index.push(results.size());
				results.emplace_back(
					scope_t{name, parameters, id, location, {}, {}, {}, {}, std::chrono::high_resolution_clock::now()});
			} // namespace internal

			void scope_results(size_t index, size_t pass, size_t fail, size_t fatal)
			{
				if(auto* scope = std::get_if<scope_t>(&results[index]); scope)
				{
					scope->pass	 = pass;
					scope->fail	 = fail;
					scope->fatal = fatal;
				}
				else
				{
					throw std::exception();
				}
			}

			void scope_close()
			{
				const auto index = active_scope_index.top();
				results.emplace_back(scope_close_t{index});
				if(auto* scope = std::get_if<scope_t>(&results[index]); scope)
				{
					scope->children		= results.size() - index - 1;
					scope->duration_end = std::chrono::high_resolution_clock::now();
				}
				else
				{
					throw std::exception();
				}
				active_scope_index.pop();
			}

			void expect_result(const std::string& lhs_value, const std::string& rhs_value, const std::string& lhs_user,
							   const std::string& rhs_user, expect_t::operation_t operation, bool pass, bool fatal,
							   const std::string& info)
			{
				const auto parent = active_scope_index.top();
				results.emplace_back(
					expect_t{lhs_value, rhs_value, lhs_user, rhs_user, operation, info,
							 ((pass) ? expect_t::result_t::pass
									 : ((fatal) ? expect_t::result_t::fatal : expect_t::result_t::fail)),
							 parent});

				if(auto* scope = std::get_if<scope_t>(&results[parent]); scope)
				{
					if(pass)
						scope->pass += 1;
					else if(fatal)
					{
						fatal = true;
						scope->fatal += 1;
					}
					else
					{
						fails = true;
						scope->fail += 1;
					}
				}
				else
				{
					throw std::exception();
				}
			}

			void calc_total(scope_t& parent, std::span<std::variant<scope_t, scope_close_t, expect_t>> children)
			{
				for(auto it = std::begin(children); it != std::end(children); ++it)
				{
					if(auto* scope = std::get_if<scope_t>(&*it); scope)
					{
						calc_total(*scope, std::span<std::variant<scope_t, scope_close_t, expect_t>>{
											   std::next(it), scope->children}); // NOLINT

						it = std::next(it, scope->children - 1); // NOLINT

						parent.pass += scope->pass;
						parent.fail += scope->fail;
						parent.fatal += scope->fatal;
					}
				}
			}

			void sync()
			{
				for(auto it = std::begin(results); it != std::end(results); ++it)
				{
					if(auto* scope = std::get_if<scope_t>(&*it); scope)
					{
						calc_total(*scope, std::span<std::variant<scope_t, scope_close_t, expect_t>>{std::next(it),
																									 scope->children});
						it = std::next(it, scope->children); // NOLINT
					}
				}
			}


			template <typename T>
			void to_string(T* logger) const
			{
				const auto* suite_scope = std::get_if<scope_t>(&results[0]);
				if(suite_scope)
				{
					if(!suite_scope->parameters.empty()) logger->suite_iterate_parameters(suite_scope->parameters);
				}
				else
					throw std::exception();

				const auto end = std::prev(std::end(results));
				for(auto it = std::next(std::begin(results)); it != end; it = std::next(it))
				{
					const auto& res = *it;
					if(const auto* scope = std::get_if<scope_t>(&res); scope)
					{
						logger->scope_begin(*scope);
					}
					else if(const auto* expect = std::get_if<expect_t>(&res); expect)
					{
						scope = std::get_if<scope_t>(&results[expect->parent_index]);
						logger->expect(*expect, *scope);
					}
					else if(const auto* scope_close = std::get_if<scope_close_t>(&res); scope_close)
					{
						scope = std::get_if<scope_t>(&results[scope_close->scope_index]);
						logger->scope_end(*scope);
					}
				}
			}

			void get_result_values(size_t& pass, size_t& fail, size_t& fatal, std::chrono::microseconds& duration) const
			{
				if(const auto* scope = std::get_if<scope_t>(&results[0]); scope)
				{
					pass	 = scope->pass;
					fail	 = scope->fail;
					fatal	 = scope->fatal;
					duration = std::chrono::duration_cast<std::chrono::microseconds>(scope->duration_end -
																					 scope->duration_start);
				}
				else
					throw std::exception();
			}

			void clear() { results.clear(); }

			auto& root() const
			{
				if(const auto* scope = std::get_if<scope_t>(&results[0]); scope)
				{
					return *scope;
				}

				throw std::exception();
			}

			bool fails{false};
			bool fatal{false};
			std::vector<std::variant<scope_t, scope_close_t, expect_t>> results{};
			std::vector<test_id_t> failed_ids{};
			std::stack<size_t> active_scope_index{};
		}; // namespace litmus

		class benchmark_result_t
		{
			struct entry_t
			{
				std::array<size_t, 100> timings{};
				size_t total_time{0};
				size_t iterations{0};
			};

		  public:
			void add(const char* name, size_t time_ns)
			{
				auto it = m_Entries.find(name);
				// if(it == std::end(m_Entries)) it = m_Entries.insert(name);

				it->second.iterations += 1;
				it->second.total_time += time_ns;
			}

		  private:
			std::unordered_map<const char*, entry_t> m_Entries{};
		};
	} // namespace internal
} // namespace litmus
