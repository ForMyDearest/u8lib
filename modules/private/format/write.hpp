#pragma once

#include "msvc_format.hpp"

#include <cmath>
#include <locale>
#include <charconv>

namespace u8lib::internal
{
	// This size is derived from the maximum length of an arithmetic type. The contenders for widest are:
	// (a) long long has a max length of 20 characters: LLONG_MIN is "-9223372036854775807".
	// (b) unsigned long long has a max length of 20 characters: ULLONG_MAX is "18446744073709551615".
	// (c) double has a max length of 24 characters: -DBL_MAX is "-1.7976931348623158e+308".
	// That's 17 characters for numeric_limits<double>::max_digits10,
	// plus 1 character for the sign,
	// plus 1 character for the decimal point,
	// plus 1 character for 'e',
	// plus 1 character for the exponent's sign,
	// plus 3 characters for the max exponent.
	inline constexpr size_t FORMAT_MIN_BUFFER_LENGTH = 24;

	inline const char8_t* measure_string_prefix(const std::u8string_view value, int& width) {
		// Returns a pointer past-the-end of the largest prefix of value that fits in width, or all
		// of value if width is negative. Updates width to the estimated width of that prefix.
		const int max_width = width;
		const auto first = value.data();
		const auto last = first + value.size();
		msvc::_Measure_string_prefix_iterator_utf pfx_iter(first, last);
		int estimated_width = 0; // the estimated width of [first, pfx_iter)

		constexpr auto max_int = std::numeric_limits<int>::max();

		while (pfx_iter != std::default_sentinel) {
			if (estimated_width == max_width && max_width >= 0) {
				// We're at our maximum length
				break;
			}

			const int character_width = *pfx_iter;

			if (max_int - character_width < estimated_width) {
				// avoid overflow
				// Either max_width isn't set, or adding this character will exceed it.
				if (max_width < 0) {
					// unset; saturate width estimate and take all characters
					width = max_int;
					return last;
				}
				break;
			}

			estimated_width += character_width;
			if (estimated_width > max_width && max_width >= 0) {
				// with this character, we exceed the maximum length
				estimated_width -= character_width;
				break;
			}
			++pfx_iter;
		}

		width = estimated_width;
		return pfx_iter.position();
	}

	template<typename T, typename Fn>
	appender write_aligned(appender out, const int width, const T& specs, const Align default_align, Fn&& func) {
		int fill_left = 0;
		int fill_right = 0;
		auto alignment = specs.alignment_;

		if (alignment == Align::none) {
			alignment = default_align;
		}

		if (width < specs.width_) {
			switch (alignment) {
				case Align::left:
					fill_right = specs.width_ - width;
					break;
				case Align::right:
					fill_left = specs.width_ - width;
					break;
				case Align::center:
					fill_left = (specs.width_ - width) / 2;
					fill_right = specs.width_ - width - fill_left;
					break;
				case Align::none:
					std::unreachable();
				default:
					break;
			}
		}

		const std::u8string_view fill_char{specs.fill_, specs.fill_length_};
		for (; fill_left > 0; --fill_left) {
			out = std::ranges::copy(fill_char, out).out;
		}

		out = func(out);

		for (; fill_right > 0; --fill_right) {
			out = std::ranges::copy(fill_char, out).out;
		}

		return out;
	}

	inline appender write_sign(appender out, const Sign sgn, const bool is_negative) {
		if (is_negative) {
			*out++ = '-';
		} else {
			switch (sgn) {
				case Sign::plus:
					*out++ = '+';
					break;
				case Sign::space:
					*out++ = ' ';
					break;
				case Sign::none:
				case Sign::minus:
					break;
			}
		}
		return out;
	}

	inline appender write_separated_integer(const char* first, const char* const last, const std::string& groups, const char separator, int separators, appender out) {
		auto group_it = groups.begin();
		auto repeats = 0;
		auto grouped = 0;

		for (int section = 0; section < separators; ++section) {
			grouped += *group_it;
			if (group_it + 1 != groups.end()) {
				++group_it;
			} else {
				++repeats;
			}
		}
		out = msvc::_Widen_and_copy<char8_t>(first, last - grouped, out);
		first = last - grouped;

		for (; separators > 0; --separators) {
			if (repeats > 0) {
				--repeats;
			} else {
				--group_it;
			}

			*out++ = separator;
			out = msvc::_Widen_and_copy<char8_t>(first, first + *group_it, out);
			first += *group_it;
		}
		assert(first == last);
		return out;
	}

	template<std::integral T>
	appender write_integral(appender out, const T value, basic_format_specs specs, const std::locale* loc) {
		if (specs.type_ == 'c') {
			if (!msvc::_In_bounds<char8_t, T>(value)) {
				report_error(u8"integral cannot be stored in char8_t");
			}
			specs.alt_ = false;
			return write(out, static_cast<char8_t>(value), specs, loc);
		}

		assert(specs.precision_ == -1);

		if (specs.sgn_ == Sign::none) {
			specs.sgn_ = Sign::minus;
		}

		int base = 10;

		switch (specs.type_) {
			case 'B':
			case 'b':
				base = 2;
				break;
			case 'X':
			case 'x':
				base = 16;
				break;
			case 'o':
				base = 8;
				break;
			default:
				break;
		}

		// long long -1 representation in binary is 64 bits + sign
		char buffer[65];
		const auto [end, ec] = std::to_chars(buffer, std::end(buffer), value, base);
		assert(ec == std::errc{});

		auto buffer_start = buffer;
		auto width = static_cast<int>(end - buffer_start);

		if (value >= T{0}) {
			if (specs.sgn_ != Sign::minus) {
				width += 1;
			}
		} else {
			// Remove the '-', it will be dealt with directly
			buffer_start += 1;
		}

		if (specs.type_ == 'X') {
			msvc::_Buffer_to_uppercase(buffer_start, end);
		}

		std::string_view prefix;
		if (specs.alt_) {
			prefix = msvc::_Get_integral_prefix(specs.type_, value);
			width += static_cast<int>(prefix.size());
		}

		auto separators = 0;
		std::string groups;
		if (specs.localized_) {
			groups = std::use_facet<std::numpunct<char>>(loc ? *loc : std::locale{}).grouping();
			separators = msvc::_Count_separators(static_cast<size_t>(end - buffer_start), groups);
			// TRANSITION, separators may be wider for wide chars
			width += separators;
		}

		const bool write_leading_zeroes = specs.leading_zero_ && specs.alignment_ == Align::none;
		auto writer = [&, end = end](appender out_) {
			out_ = internal::write_sign(out_, specs.sgn_, value < T{0});
			out_ = msvc::_Widen_and_copy<char8_t>(prefix.data(), prefix.data() + prefix.size(), out_);
			if (write_leading_zeroes && width < specs.width_) {
				out_ = std::ranges::fill_n(out_, specs.width_ - width, '0');
			}

			if (separators > 0) {
				return internal::write_separated_integer(
					buffer_start,
					end,
					groups,
					std::use_facet<std::numpunct<char>>(loc ? *loc : std::locale{}).thousands_sep(),
					separators,
					out_
				);
			}
			return msvc::_Widen_and_copy<char8_t>(buffer_start, end, out_);
		};

		if (write_leading_zeroes) {
			return writer(out);
		}

		return internal::write_aligned(out, width, specs, Align::right, writer);
	}
}

namespace u8lib::internal
{
	inline appender write(appender, std::monostate) {
		std::unreachable();
	}

	template<typename T> requires(std::is_arithmetic_v<T> && !is_any_of_v<T, char8_t, bool>)
	appender write(appender out, const T value) {
		// TRANSITION, Reusable buffer
		char buffer[FORMAT_MIN_BUFFER_LENGTH];
		char* end = buffer;

		if constexpr (std::is_floating_point_v<T>) {
			if (std::isnan(value)) {
				if (std::signbit(value)) {
					*end++ = '-';
				}

				*end++ = 'n';
				*end++ = 'a';
				*end++ = 'n';
			}
		}

		if (end == buffer) {
			const std::to_chars_result result = std::to_chars(buffer, std::end(buffer), value);
			assert(result.ec == std::errc{});
			end = result.ptr;
		}

		return msvc::_Widen_and_copy<char8_t>(buffer, end, out);
	}

	inline appender write(appender out, const bool value) {
		return write(out, value ? u8"true" : u8"false");
	}

	inline appender write(appender out, const char8_t value) {
		*out++ = value;
		return out;
	}

	inline appender write(appender out, const void* const value) {
		// TRANSITION, Reusable buffer
		char buffer[FORMAT_MIN_BUFFER_LENGTH];
		const auto [end, ec] = std::to_chars(buffer, std::end(buffer), reinterpret_cast<uintptr_t>(value), 16);
		assert(ec == std::errc{});
		*out++ = '0';
		*out++ = 'x';
		return msvc::_Widen_and_copy<char8_t>(buffer, end, out);
	}

	inline appender write(appender out, const char8_t* value) {
		if (!value) {
			report_error(u8"string pointer is null.");
		}

		while (*value) {
			*out++ = *value++;
		}

		return out;
	}

	inline appender write(appender out, const std::u8string_view value) {
		return std::ranges::copy(value, out).out;
	}

	inline appender write(appender, std::monostate, const basic_format_specs&, const std::locale*) {
		std::unreachable();
	}

	template<std::integral T> requires(!is_any_of_v<T, char8_t, bool>)
	appender write(appender out, const T value, const basic_format_specs& specs, const std::locale* loc) {
		return internal::write_integral(out, value, specs, loc);
	}

	inline appender write(appender out, const bool value, basic_format_specs specs, const std::locale* loc) {
		if (specs.type_ != '\0' && specs.type_ != 's') {
			return write_integral(out, static_cast<unsigned char>(value), specs, loc);
		}

		assert(specs.precision_ == -1);

		if (specs.localized_) {
			specs.localized_ = false;

			const auto& facet = std::use_facet<std::numpunct<char>>(loc ? *loc : std::locale{});
			auto sv = value
						? static_cast<std::string_view>(facet.truename())
						: static_cast<std::string_view>(facet.falsename());
			return write(out, std::u8string_view(reinterpret_cast<const char8_t*>(sv.data()), sv.size()), specs, loc);
		}

		return write(out, value ? u8"true" : u8"false", specs, loc);
	}

	inline appender write_escaped(appender out, std::u8string_view value, char8_t delim) {
		auto first = value.data();
		const auto last = first + value.size();

		assert(delim == '"' || delim == '\'');
		*out++ = delim;

		bool escape_grapheme_extend = true;
		char buffer[8];

		while (first != last) {
			const auto ch = *first;

			if (ch == '\t') {
				out = write(out, u8R"(\t)");
				escape_grapheme_extend = true;
				++first;
			} else if (ch == '\n') {
				out = write(out, u8R"(\n)");
				escape_grapheme_extend = true;
				++first;
			} else if (ch == '\r') {
				out = write(out, u8R"(\r)");
				escape_grapheme_extend = true;
				++first;
			} else if (ch == delim) {
				*out++ = '\\';
				*out++ = delim;
				escape_grapheme_extend = true;
				++first;
			} else if (ch == '\\') {
				out = write(out, u8R"(\\)");
				escape_grapheme_extend = true;
				++first;
			} else {
				char32_t decoded_ch;
				const auto [next, is_usv] = decode_utf(first, last, decoded_ch);

				if (is_usv) {
					const bool needs_escape = !msvc::_Is_printable(decoded_ch) || (escape_grapheme_extend && msvc::_Is_grapheme_extend(decoded_ch));

					if (needs_escape) {
						out = write(out, u8R"(\u{)");

						const auto [end, ec] = std::to_chars(buffer, std::end(buffer), static_cast<uint32_t>(decoded_ch), 16);
						assert(ec == std::errc{});

						out = msvc::_Widen_and_copy<char8_t>(buffer, end, out);

						*out++ = '}';
						escape_grapheme_extend = true;
					} else {
						out = std::copy(first, next, out);
						escape_grapheme_extend = false;
					}

					first = next;
				} else {
					for (; first != next; ++first) {
						out = write(out, u8R"(\x{)");

						const auto [end, ec] = std::to_chars(buffer, std::end(buffer), static_cast<std::make_unsigned_t<char8_t>>(*first), 16);
						assert(ec == std::errc{});

						out = msvc::_Widen_and_copy<char8_t>(buffer, end, out);

						*out++ = '}';
					}
					escape_grapheme_extend = true;
				}
			}
		}

		*out++ = delim;

		return out;
	}

	inline appender write_escaped(appender out, std::u8string_view value, basic_format_specs specs, char8_t delim) {
		if (specs.precision_ < 0 && specs.width_ <= 0) {
			return write_escaped(out, value, delim);
		}

		std::u8string temp;
		if (true) {
			iterator_buffer<std::back_insert_iterator<std::u8string>, buffer_traits> buf(std::back_inserter(temp));
			write_escaped(appender{buf}, value, delim);
		}

		int width = specs.precision_;
		const char8_t* last = measure_string_prefix(temp, width);

		return write_aligned(out, width, specs, Align::left, [&temp, last](appender out_) {
			return write(out_, std::u8string_view{temp.data(), last});
		});
	}

	inline appender write(appender out, const char8_t value, basic_format_specs specs, const std::locale* loc) {
		if (specs.type_ != '\0' && specs.type_ != 'c' && specs.type_ != '?') {
			return write_integral(out, value, specs, loc);
		}

		assert(specs.precision_ == -1);

		if (specs.type_ == '?') {
			return write_escaped(out, std::u8string_view{&value, 1}, specs, '\'');
		}

		return write(out, std::u8string_view{&value, 1}, specs, loc);
	}

	template<std::floating_point T>
	appender write(appender out, const T value, const basic_format_specs& specs, const std::locale* loc) {
		auto sgn = specs.sgn_;
		if (sgn == Sign::none) {
			sgn = Sign::minus;
		}

		auto to_upper = false;
		auto format = std::chars_format::general;
		auto exponent = 'e';
		auto precision = specs.precision_;

		switch (specs.type_) {
			case 'A':
				to_upper = true;
				[[fallthrough]];
			case 'a':
				format = std::chars_format::hex;
				exponent = 'p';
				break;
			case 'E':
				to_upper = true;
				[[fallthrough]];
			case 'e':
				if (precision == -1) {
					precision = 6;
				}
				format = std::chars_format::scientific;
				break;
			case 'F':
				to_upper = true;
				[[fallthrough]];
			case 'f':
				if (precision == -1) {
					precision = 6;
				}
				format = std::chars_format::fixed;
				break;
			case 'G':
				to_upper = true;
				[[fallthrough]];
			case 'g':
				if (precision == -1) {
					precision = 6;
				}
				format = std::chars_format::general;
				break;
			default:
				break;
		}

		// Consider the powers of 2 in decimal:
		// 2^-1 = 0.5
		// 2^-2 = 0.25
		// 2^-3 = 0.125
		// 2^-4 = 0.0625
		// Each power of 2 consumes one more decimal digit. This is because:
		// 2^-N * 5^-N = 10^-N
		// 2^-N = 10^-N * 5^N
		// Example: 2^-4 = 10^-4 * 5^4 = 0.0001 * 625
		// Therefore, the min subnormal 2^-1074 consumes 1074 digits of precision (digits after the decimal point).
		// We need 3 more characters for a potential negative sign, the zero integer part, and the decimal point.
		// Therefore, the precision can be clamped to 1074.
		// The largest number consumes 309 digits before the decimal point. With a precision of 1074, and it being
		// negative, it would use a buffer of size 1074+309+2. We need to add an additional number to the max
		// exponent to accommodate the ones place.
		constexpr auto MAX_PRECISION = 1074;
		constexpr auto BUFFER_SIZE = MAX_PRECISION + DBL_MAX_10_EXP + 3;
		char buffer[BUFFER_SIZE];
		std::to_chars_result result;

		auto extra_precision = 0;
		if (precision > MAX_PRECISION) {
			extra_precision = precision - MAX_PRECISION;
			precision = MAX_PRECISION;
		}

		const auto is_negative = std::signbit(value);

		if (std::isnan(value)) {
			result.ptr = buffer;
			if (is_negative) {
				++result.ptr; // pretend to skip over a '-' that to_chars would put in buffer[0]
			}
			*result.ptr++ = 'n';
			*result.ptr++ = 'a';
			*result.ptr++ = 'n';
		} else {
			if (precision == -1) {
				result = std::to_chars(buffer, std::end(buffer), value, format);
			} else {
				result = std::to_chars(buffer, std::end(buffer), value, format, precision);
			}

			assert(result.ec == std::errc{});
		}

		auto buffer_start = buffer;
		auto width = static_cast<int>(result.ptr - buffer_start);

		if (is_negative) {
			// Remove the '-', it will be dealt with directly
			buffer_start += 1;
		} else {
			if (sgn != Sign::minus) {
				width += 1;
			}
		}

		assert(exponent == 'e' || exponent == 'p');
		if (to_upper) {
			msvc::_Buffer_to_uppercase(buffer_start, result.ptr);
			exponent -= 'a' - 'A';
		}

		const auto is_finite = std::isfinite(value);

		auto append_decimal = false;
		auto exponent_start = result.ptr;
		auto radix_point = result.ptr;
		auto integral_end = result.ptr;
		auto zeroes_to_append = 0;
		auto separators = 0;
		std::string groups;

		if (is_finite) {
			if (specs.alt_ || specs.localized_) {
				for (auto iter = buffer_start; iter < result.ptr; ++iter) {
					if (*iter == '.') {
						radix_point = iter;
					} else if (*iter == exponent) {
						exponent_start = iter;
					}
				}
				integral_end = std::min(radix_point, exponent_start);

				if (specs.alt_ && radix_point == result.ptr) {
					// TRANSITION, decimal point may be wider
					++width;
					append_decimal = true;
				}

				if (specs.localized_) {
					groups = std::use_facet<std::numpunct<char>>(loc ? *loc : std::locale{}).grouping();
					separators = msvc::_Count_separators(static_cast<size_t>(integral_end - buffer_start), groups);
				}
			}

			switch (format) {
				case std::chars_format::hex:
				case std::chars_format::scientific:
					if (extra_precision != 0) {
						// Trailing zeroes are in front of the exponent
						while (*--exponent_start != exponent) {}
					}
					[[fallthrough]];
				case std::chars_format::fixed:
					zeroes_to_append = extra_precision;
					break;
				case std::chars_format::general:
					if (specs.alt_ && (specs.type_ == 'g' || specs.type_ == 'G')) {
						auto digits = static_cast<int>(exponent_start - buffer_start);

						if (!append_decimal) {
							--digits;
						}

						zeroes_to_append = extra_precision + precision - digits;

						// Leading zeroes are not significant if we used fixed point notation.
						if (exponent_start == result.ptr && std::abs(value) < 1.0 && value != 0.0) {
							for (auto iter = buffer_start; iter < result.ptr; ++iter) {
								if (*iter == '0') {
									++zeroes_to_append;
								} else if (*iter != '.') {
									break;
								}
							}
						}
					}
					break;
				default:
					std::unreachable();
			}
		}

		width += zeroes_to_append;

		const bool write_leading_zeroes = specs.leading_zero_ && specs.alignment_ == Align::none && is_finite;

		auto writer = [&](appender out_) {
			out_ = internal::write_sign(out_, sgn, is_negative);

			if (write_leading_zeroes && width < specs.width_) {
				out_ = std::ranges::fill_n(out_, specs.width_ - width, '0');
			}

			if (specs.localized_) {
				const auto& facet = std::use_facet<std::numpunct<char>>(loc ? *loc : std::locale{});

				out_ = write_separated_integer(buffer_start, integral_end, groups, facet.thousands_sep(), separators, out_);
				if (radix_point != result.ptr || append_decimal) {
					*out_++ = facet.decimal_point();
					append_decimal = false;
				}
				buffer_start = integral_end;
				if (radix_point != result.ptr) {
					++buffer_start;
				}
			}

			out_ = msvc::_Widen_and_copy<char8_t>(buffer_start, exponent_start, out_);
			if (specs.alt_ && append_decimal) {
				*out_++ = '.';
			}

			for (; zeroes_to_append > 0; --zeroes_to_append) {
				*out_++ = '0';
			}

			return msvc::_Widen_and_copy<char8_t>(exponent_start, result.ptr, out_);
		};

		if (write_leading_zeroes) {
			return writer(out);
		}

		return internal::write_aligned(out, width, specs, Align::right, writer);
	}

	appender write(appender out, const void* const value, const basic_format_specs& specs, const std::locale*) {
		assert(specs.type_ == '\0' || specs.type_ == 'p');
		assert(specs.sgn_ == Sign::none);
		assert(!specs.alt_);
		assert(specs.precision_ == -1);
		assert(!specs.leading_zero_);
		assert(!specs.localized_);

		// Since the bit width of 0 is 0x0, special-case it instead of complicating the math even more.
		int width = 3;
		if (value != nullptr) {
			// Compute the bit width of the pointer (i.e. how many bits it takes to be represented).
			// Add 3 to the bit width so we always round up on the division.
			// Divide that by the amount of bits a hexit represents (log2(16) = log2(2^4) = 4).
			// Add 2 for the 0x prefix.
			width = 2 + (std::bit_width(reinterpret_cast<uintptr_t>(value)) + 3) / 4;
		}

		return write_aligned(out, width, specs, Align::right, [=](appender out_) {
			return write(out_, value);
		});
	}

	appender write(appender out, const char8_t* value, const basic_format_specs& specs, const std::locale* loc) {
		return write(out, std::u8string_view{value}, specs, loc);
	}

	appender write(appender out, const std::u8string_view value, const basic_format_specs& specs, const std::locale*) {
		assert(specs.type_ == '\0' || specs.type_ == 'c' || specs.type_ == 's' || specs.type_ == '?');
		assert(specs.sgn_ == Sign::none);
		assert(!specs.alt_);
		assert(!specs.leading_zero_);

		if (specs.type_ == '?') {
			return write_escaped(out, value, specs, '"');
		}

		if (specs.precision_ < 0 && specs.width_ <= 0) {
			return write(out, value);
		}

		int width = specs.precision_;
		const char8_t* last = measure_string_prefix(value, width);

		return write_aligned(out, width, specs, Align::left, [=](appender out_) {
			return write(out_, std::u8string_view{value.data(), last});
		});
	}
}
