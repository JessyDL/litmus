#pragma once
#include <cstddef>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include "exceptions.hpp"

namespace litmus
{
	inline namespace internal
	{
		class file_t
		{
		  public:
			file_t(const std::string& filename);

			auto substr(size_t line, size_t offset = 0) const -> std::string_view
			{
				except(line > m_LinePos.size() || m_LinePos[line] + offset > m_Content.size(), std::range_error{"Out of bounds access"});
				return static_cast<std::string_view>(m_Content).substr(m_LinePos[line] + offset);
			}
		  private:
			std::string m_Content;
			std::vector<size_t> m_LinePos;
			std::vector<size_t> m_LineLength;
			std::vector<std::string_view> m_Lines{};
		};

		class cache_t
		{
			struct data_t
			{
				std::unordered_map<std::string, file_t> files;
				std::mutex mutex;
			};

		  public:
			cache_t()				= default;
			cache_t(cache_t const&) = delete;
			cache_t(cache_t&&)		= delete;

			auto operator=(cache_t const&) -> cache_t& = delete;
			auto operator=(cache_t &&) -> cache_t& = delete;


			const file_t& get(const std::string& file) const;

		  private:
			mutable data_t m_Data{};
		};

		extern cache_t cache;
	} // namespace internal
} // namespace litmus