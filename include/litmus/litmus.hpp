#pragma once

#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <litmus/details/verbosity.hpp>
#include <litmus/details/cache.hpp>


namespace litmus
{
	class formatter;

	inline namespace internal
	{
		struct config_t
		{

			struct data_t
			{
				bool no_source{false};
				std::string source{};
				size_t source_size_limit{80u};
				std::vector<std::string> categories{};
				verbosity_t verbosity{verbosity_t::NORMAL};
				bool rerun_failed{false};
				bool single_threaded{false};
				bool break_on_fatal{false};
				bool break_on_fail{false};
			} data{};

			config_t()				  = default;
			config_t(config_t const&) = delete;
			config_t(config_t&&)	  = delete;

			auto operator=(config_t const&) -> config_t& = delete;
			auto operator=(config_t&&) -> config_t&		 = delete;

			[[nodiscard]] auto iverbosity() const noexcept -> std::underlying_type_t<verbosity_t>
			{
				return static_cast<std::underlying_type_t<verbosity_t>>(data.verbosity);
			}
			[[nodiscard]] auto configured() const noexcept { return !data.source.empty(); }

			auto operator->() const -> data_t const* { return &data; }
			auto operator->() -> data_t* { return &data; }
		};

		extern config_t config;
	} // namespace internal

	// NOLINTNEXTLINE
	auto run(int argc, char* argv[], formatter* formatter = nullptr) noexcept -> int;
} // namespace litmus


#if defined(LITMUS_INCLUDE_ALL) || defined(LITMUS_FULL)
#include <litmus/suite.hpp>
#include <litmus/section.hpp>
#include <litmus/expect.hpp>
#endif

#define LITMUS_EXTERN()                                                                                                \
	litmus::internal::runner_t litmus::internal::runner{};                                                             \
	litmus::internal::cache_t litmus::internal::cache{};                                                               \
	litmus::internal::config_t litmus::internal::config {}

#define LITMUS_MAIN()                                                                                                  \
	LITMUS_EXTERN();                                                                                                   \
	int main(int argc, char* argv[]) { return litmus::run(argc, argv); }

#if defined(LITMUS_FULL)
LITMUS_MAIN()
#endif
