#pragma once

#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <litmus/details/verbosity.hpp>


namespace litmus
{
	class formatter;

	inline namespace internal
	{
		struct config_t
		{

			static struct data_t
			{
				bool no_source{false};
				std::string source{};
				size_t source_size_limit{80u};
				std::vector<std::string> categories{};
				verbosity_t verbosity{verbosity_t::NORMAL};
				bool rerun_failed{false};
				bool single_threaded{false};
			} * data;

			config_t()
			{
				if(m_RefCount == 0) data = new data_t();
				++m_RefCount;
			}

			~config_t()
			{
				if(--m_RefCount == 0) delete(data);
			}

			config_t(config_t const&) = delete;
			config_t(config_t&&)	  = delete;

			auto operator=(config_t const&) -> config_t& = delete;
			auto operator=(config_t &&) -> config_t& = delete;

			[[nodiscard]] auto iverbosity() const noexcept -> std::underlying_type_t<verbosity_t>
			{
				return static_cast<std::underlying_type_t<verbosity_t>>(data->verbosity);
			}
			[[nodiscard]] auto configured() const noexcept { return !data->source.empty(); }

			auto operator-> () const -> data_t* { return data; }

			static unsigned m_RefCount;
		};

		config_t const config;
	} // namespace internal

	// NOLINTNEXTLINE
	auto run(int argc, char* argv[], formatter* formatter = nullptr) noexcept -> int;
} // namespace litmus


#if defined(LITMUS_INCLUDE_ALL) || defined(LITMUS_FULL)
#include <litmus/suite.hpp>
#include <litmus/section.hpp>
#include <litmus/expect.hpp>
#endif

#define LITMUS_MAIN()                                                                                                  \
	int main(int argc, char* argv[]) { return litmus::run(argc, argv); }

#if defined(LITMUS_FULL)
LITMUS_MAIN()
#endif
