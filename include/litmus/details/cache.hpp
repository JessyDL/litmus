#pragma once
#include <cstddef>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace litmus
{
	inline namespace internal
	{
		class file_t
		{
		  public:
			file_t(const std::string& filename);
			auto begin() noexcept -> std::vector<std::string_view>::iterator { return std::begin(m_Lines); }
			auto begin() const noexcept -> std::vector<std::string_view>::const_iterator { return std::begin(m_Lines); }
			auto cbegin() const noexcept -> std::vector<std::string_view>::const_iterator
			{
				return std::cbegin(m_Lines);
			}
			auto end() noexcept -> std::vector<std::string_view>::iterator { return std::end(m_Lines); }
			auto end() const noexcept -> std::vector<std::string_view>::const_iterator { return std::end(m_Lines); }
			auto cend() const noexcept -> std::vector<std::string_view>::const_iterator { return std::cend(m_Lines); }

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
			cache_t()
			{
				if(m_RefCount == 0) m_Data = new data_t();
				++m_RefCount;
			}

			~cache_t()
			{
				if(--m_RefCount == 0) delete(m_Data);
			}

			cache_t(cache_t const&) = delete;
			cache_t(cache_t&&)		= delete;

			auto operator=(cache_t const&) -> cache_t& = delete;
			auto operator=(cache_t &&) -> cache_t& = delete;


			const file_t& get(const std::string& file) const;

		  private:
			static data_t* m_Data;
			static unsigned m_RefCount;
		};

		const cache_t cache;
	} // namespace internal
} // namespace litmus