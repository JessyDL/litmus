#pragma once
#include <functional>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include <litmus/details/test_result.hpp>

namespace litmus
{
	template <typename T>
	inline auto type_to_name(std::type_identity<T>) -> std::string
	{
		return typeid(T).name();
	}

	template <>
	inline auto type_to_name(std::type_identity<uint8_t>) -> std::string
	{
		return "ui8";
	}
	template <>
	inline auto type_to_name(std::type_identity<uint16_t>) -> std::string
	{
		return "ui16";
	}
	template <>
	inline auto type_to_name(std::type_identity<uint32_t>) -> std::string
	{
		return "ui32";
	}
	template <>
	inline auto type_to_name(std::type_identity<uint64_t>) -> std::string
	{
		return "ui64";
	}
	template <>
	inline auto type_to_name(std::type_identity<int8_t>) -> std::string
	{
		return "i8";
	}
	template <>
	inline auto type_to_name(std::type_identity<int16_t>) -> std::string
	{
		return "i16";
	}
	template <>
	inline auto type_to_name(std::type_identity<int32_t>) -> std::string
	{
		return "i32";
	}
	template <>
	inline auto type_to_name(std::type_identity<int64_t>) -> std::string
	{
		return "i64";
	}
	template <>
	inline auto type_to_name(std::type_identity<float>) -> std::string
	{
		return "float";
	}
	template <>
	inline auto type_to_name(std::type_identity<double>) -> std::string
	{
		return "double";
	}
	template <>
	inline auto type_to_name(std::type_identity<bool>) -> std::string
	{
		return "bool";
	}

	inline namespace internal
	{
		template <typename T>
		inline auto type_to_name_internal() -> std::string
		{
			return type_to_name(std::type_identity<std::remove_cvref_t<T>>{});
		}

		template <typename... Ts>
		constexpr const std::uintptr_t uuid_var{0u};

		template <typename... Ts>
		constexpr const std::uintptr_t* uuid() noexcept
		{
			return &uuid_var<Ts...>;
		}


		using uuid_t = const std::uintptr_t* (*)();

		template <typename... Ts>
		constexpr uuid_t uuid_for()
		{
			return uuid<typename std::decay<Ts>::type...>;
		};

		struct runner_t
		{
		  public:
			struct template_pack_t
			{
				std::vector<std::string> templates{};
				std::vector<std::function<test_result_t()>> functions{};
			};

			using test_t = std::unordered_map<uuid_t, template_pack_t>;

			runner_t()
			{
				if(m_RefCount == 0)
				{
					m_NamedTests = new std::unordered_map<const char*, test_t>();
				}
				++m_RefCount;
			}

			~runner_t()
			{
				if(--m_RefCount == 0)
				{
					delete(m_NamedTests);
				}
			}

			runner_t(runner_t const&) = delete;
			runner_t(runner_t&&)	  = delete;

			auto operator=(runner_t const&) -> runner_t& = delete;
			auto operator=(runner_t &&) -> runner_t& = delete;

			[[nodiscard]] static auto begin() noexcept { return m_NamedTests->begin(); }
			[[nodiscard]] static auto cbegin() noexcept { return m_NamedTests->cbegin(); }

			[[nodiscard]] static auto end() noexcept { return m_NamedTests->end(); }
			[[nodiscard]] static auto cend() noexcept { return m_NamedTests->cend(); }

			[[nodiscard]] static auto size() noexcept { return m_NamedTests->size(); }

			template <typename... Ts>
			static void test(const char* name, auto&& arg)
			{
				auto& t = (*m_NamedTests)[name][uuid_for<Ts...>()];
				if constexpr(sizeof...(Ts) > 0)
				{
					if(t.templates.size() != sizeof...(Ts))
						t.templates = std::vector<std::string>{{type_to_name_internal<Ts>()}...};
				}
				t.functions.emplace_back(std::forward<decltype(arg)>(arg));
			}

			template <typename T>
			static void benchmark(T&& arg)
			{
				m_Benchmarks->emplace_back(std::forward<T>(arg));
			}

		  private:
			static std::unordered_map<const char*, test_t>* m_NamedTests;
			static std::vector<std::function<void()>>* m_Benchmarks;
			static unsigned m_RefCount;
		};

		runner_t const runner;
	} // namespace internal
} // namespace litmus
