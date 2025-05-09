#pragma once

#include "string.hpp"

#include <locale>

namespace u8lib
{
	//=====================> format_to <========================

	template<std::output_iterator<const char8_t&> OutputIt>
	OutputIt vformat_to(OutputIt out, const std::locale& loc, std::u8string_view fmt, format_args args) {
		auto buf = internal::iterator_buffer<OutputIt, internal::buffer_traits>(out);
		internal::vformat_to(buf, fmt, args, &loc);
		return buf.out();
	}

	inline char8_t* vformat_to(char8_t* out, const std::locale& loc, std::u8string_view fmt, format_args args) {
		return internal::vformat_to(out, fmt, args, &loc);
	}

	template<std::output_iterator<const char8_t&> OutputIt, typename... T>
	OutputIt format_to(OutputIt out, const std::locale& loc, format_string<T...> fmt, T&&... args) {
		constexpr auto DESC = internal::make_descriptor<T...>();
		return u8lib::vformat_to(out, loc, fmt.get(), format_args(u8lib::make_format_store(args...), DESC));
	}

	//====================> format_to_n <=======================

	template<std::output_iterator<const char8_t&> OutputIt>
	format_to_n_result<OutputIt> vformat_to_n(OutputIt out, size_t n, const std::locale& loc, std::u8string_view fmt, format_args args) {
		auto buf = internal::iterator_buffer<OutputIt, internal::fixed_buffer_traits>(out, n);
		internal::vformat_to(buf, fmt, args, &loc);
		return {buf.out(), buf.count()};
	}

	inline format_to_n_result<char8_t*> vformat_to_n(char8_t* out, size_t n, const std::locale& loc, std::u8string_view fmt, format_args args) {
		return internal::vformat_to_n(out, n, fmt, args, &loc);
	}

	template<std::output_iterator<const char8_t&> OutputIt, typename... T>
	format_to_n_result<OutputIt> format_to_n(OutputIt out, size_t n, const std::locale& loc, format_string<T...> fmt, T&&... args) {
		constexpr auto DESC = internal::make_descriptor<T...>();
		return u8lib::vformat_to_n(out, n, loc, fmt.get(), format_args(u8lib::make_format_store(args...), DESC));
	}

	//===============> format_to fixed array <==================

	template<size_t N>
	format_to_result vformat_to(char8_t (&out)[N], const std::locale& loc, std::u8string_view fmt, format_args args) {
		auto result = u8lib::vformat_to_n(out, N, fmt, args, &loc);
		return {result.out, result.size > N};
	}

	template<size_t N, typename... T>
	format_to_result format_to(char8_t (&out)[N], const std::locale& loc, format_string<T...> fmt, T&&... args) {
		constexpr auto DESC = internal::make_descriptor<T...>();
		return u8lib::vformat_to(out, loc, fmt.get(), format_args(u8lib::make_format_store(args...), DESC));
	}

	//===================> formatted_size <=====================

	inline size_t vformatted_size(const std::locale& loc, std::u8string_view fmt, format_args args) {
		return internal::vformatted_size(fmt, args, &loc);
	}

	template<typename... T>
	size_t formatted_size(const std::locale& loc, format_string<T...> fmt, T&&... args) {
		constexpr auto DESC = internal::make_descriptor<T...>();
		return u8lib::vformatted_size(loc, fmt.get(), format_args(u8lib::make_format_store(args...), DESC));
	}

	//=======================> format <=========================

	inline u8string vformat(const std::locale& loc, std::u8string_view fmt, format_args args) {
		return internal::vformat(fmt, args, &loc);
	}

	template<typename... T>
	u8string format(const std::locale& loc, format_string<T...> fmt, T&&... args) {
		constexpr auto DESC = internal::make_descriptor<T...>();
		return u8lib::vformat(loc, fmt.get(), format_args(u8lib::make_format_store(args...), DESC));
	}
}
