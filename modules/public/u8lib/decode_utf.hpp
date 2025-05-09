#pragma once

#include <cassert>

namespace u8lib
{
	struct decode_result {
		const char8_t* next_ptr_;
		bool is_unicode_scalar_value_; // Also _Is_usv below, see https://www.unicode.org/glossary/#unicode_scalar_value
		// is_unicode_scalar_value_ is also used for non-Unicode encodings, to indicate that the input can be converted to
		// Unicode.
	};

	constexpr decode_result decode_utf(const char8_t* first, const char8_t* last, char32_t& value) noexcept {
		assert(first < last);
		// Decode a UTF-8 encoded codepoint starting at first and not exceeding last, returning
		// one past the end of the character decoded. Any invalid codepoints will result in
		// value == U+FFFD and _Decode_utf will return one past the
		// maximal subpart of the ill-formed subsequence. So, most invalid UTF-8 will result in
		// one U+FFFD for each byte of invalid data. Truncated but otherwise valid UTF-8 may
		// result in one U+FFFD for more than one input byte.
		value = static_cast<char32_t>(static_cast<unsigned char>(*first));

		// All UTF-8 text is at least one byte.
		// The zero extended values of the "prefix" bytes for
		// a multi-byte sequence are the lowest numeric value (in two's complement)
		// that any leading byte could have for a code unit of that size, so
		// we just sum the comparisons to get the number of trailing bytes.
		int num_bytes;
		if (value <= 0x7F) {
			return {first + 1, true};
		}
		if (value >= 0xC2 && value <= 0xDF) {
			num_bytes = 2;
		} else if (value >= 0xE0 && value <= 0xEF) {
			num_bytes = 3;
		} else if (value >= 0xF0 && value <= 0xF4) {
			num_bytes = 4;
		} else {
			// definitely not valid
			value = 0xFFFD;
			return {first + 1, false};
		}

		if (first + 1 == last) {
			// We got a multibyte sequence and the next byte is off the end, we need
			// to check just the next byte here since we need to look for overlong sequences.
			// We want to return one past the end of a truncated sequence if everything is
			// otherwise valid, so we can't check if first + num_bytes is off the end.
			value = 0xFFFD;
			return {last, false};
		}

		switch (value) {
			case 0xE0:
				// we know first[1] is in range because we just checked above,
				// and a leader of 0xE0 implies num_bytes == 3
				if (static_cast<unsigned char>(first[1]) < 0xA0) {
					// note, we just increment forward one-byte,
					// even though num_bytes would imply the next
					// codepoint starts at first + 2, this is because
					// we don't consume trailing bytes of ill-formed subsequences
					value = 0xFFFD;
					return {first + 1, false};
				}
				break;
			case 0xED:
				if (static_cast<unsigned char>(first[1]) > 0x9F) {
					value = 0xFFFD;
					return {first + 1, false};
				}
				break;
			case 0xF0:
				if (static_cast<unsigned char>(first[1]) < 0x90) {
					value = 0xFFFD;
					return {first + 1, false};
				}
				break;
			case 0xF4:
				if (static_cast<unsigned char>(first[1]) > 0x8F) {
					value = 0xFFFD;
					return {first + 1, false};
				}
				break;
			default:
				break;
		}

		// mask out the "value bits" in the leading byte,
		// for one-byte codepoints there is no leader,
		// two-byte codepoints have the same number of value
		// bits as trailing bytes (including the leading zero)
		switch (num_bytes) {
			case 2:
				value &= 0b1'1111u;
				break;
			case 3:
				value &= 0b1111u;
				break;
			case 4:
				value &= 0b111u;
				break;
			default:
				break;
		}

		for (int idx = 1; idx < num_bytes; ++idx) {
			if (
				first + idx >= last ||
				static_cast<unsigned char>(first[idx]) < 0x80 ||
				static_cast<unsigned char>(first[idx]) > 0xBF
			) {
				// truncated sequence
				value = 0xFFFD;
				return {first + idx, false};
			}
			// we know we're always in range due to the above check.
			value = (value << 6) | (static_cast<unsigned char>(first[idx]) & 0b11'1111u);
		}
		return {first + num_bytes, true};
	}

	constexpr int utf8_code_units_in_next_character(const char8_t* const first, const char8_t* const last) noexcept {
		char32_t ch;
		const auto [next, is_usv] = decode_utf(first, last, ch);
		assert(next - first <= 4);
		return is_usv ? static_cast<int>(next - first) : -1;
	}
}
