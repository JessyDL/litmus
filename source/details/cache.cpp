#include <litmus/details/cache.hpp>
#include <litmus/details/source_location.hpp>

unsigned litmus::internal::cache_t::m_RefCount						 = 0;
litmus::internal::cache_t::data_t* litmus::internal::cache_t::m_Data = nullptr;

#include <algorithm>
#include <fstream>

using namespace litmus::internal;

file_t::file_t(const std::string& filename)
{
#ifndef LITMUS_NO_SOURCE
	std::ifstream stream(filename);
	if(!stream.is_open())
	{
		throw std::runtime_error("could not open file" + filename);
	}

	m_Content = std::string{(std::istreambuf_iterator<char>(stream)), (std::istreambuf_iterator<char>())};

	auto lineCount = std::count_if(std::begin(m_Content), std::end(m_Content), [](auto ch) { return ch == '\n'; }) + 1;

	m_LinePos.reserve(lineCount);
	m_LineLength.reserve(lineCount);
	m_Lines.reserve(lineCount);
	m_LinePos.emplace_back(0u);
	size_t index{0u};

	for(auto ch : m_Content)
	{
		if(ch == '\n' || ch == '\r')
		{
			m_LineLength.emplace_back(index - m_LinePos.back());
			m_Lines.emplace_back(&m_Content[m_LinePos.back()], m_LineLength.back());
			if(ch == '\r')
			{
				m_LinePos.emplace_back(index + 2);
				++index;
			}
			else
			{
				m_LinePos.emplace_back(index + 1);
			}
		}
		++index;
	}
	m_LineLength.emplace_back(m_Content.size() - m_LinePos.back());
	m_Lines.emplace_back(&m_Content[m_LinePos.back()], m_LineLength.back());
	stream.close();
#endif
}

const file_t& cache_t::get(const std::string& file) const
{
	std::scoped_lock lock{m_Data->mutex};
	return m_Data->files.try_emplace(file, file).first->second;
}