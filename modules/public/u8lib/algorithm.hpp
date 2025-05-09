#pragma once

#include <array>
#include <cassert>
#include <concepts>

namespace u8lib
{
	template<typename T, typename... Ts>
	inline constexpr bool is_any_of_v = (std::is_same_v<T, Ts> || ...);

	template<typename T>
	concept is_char_v = is_any_of_v<T, char, char8_t, char16_t, char32_t, wchar_t>;
}

namespace u8lib
{
	//==================> constants <==================
	static constexpr char32_t kUtf16LeadingSurrogateHeader = 0xD800;
	static constexpr char32_t kUtf16TrailingSurrogateHeader = 0xDC00;
	static constexpr char32_t kUtf16LeadingSurrogateMin = 0xD800;
	static constexpr char32_t kUtf16LeadingSurrogateMax = 0xDBFF;
	static constexpr char32_t kUtf16TrailingSurrogateMin = 0xDC00;
	static constexpr char32_t kUtf16TrailingSurrogateMax = 0xDFFF;
	static constexpr char32_t kUtf16SurrogateMask = 0x03FF;
	static constexpr char32_t kBMPMaxCodePoint = 0xFFFF;
	static constexpr char32_t kSMPBaseCodePoint = 0x10000;

	//==================> utf-8 <==================
	// return maximum code point for given sequence length
	constexpr char32_t utf8_maximum_code_point(uint64_t seq_len);
	// seq_len: 0 => trailing mask, 1-4 => leading mask
	constexpr char8_t utf8_mask(uint64_t seq_len);
	// return: 0 => ch is trailing, 1-4 => sequence length (if ch is leading)
	constexpr uint64_t utf8_seq_len(char8_t ch);
	// return: 1-4 => sequence length
	constexpr uint64_t utf8_seq_len(char16_t ch);
	// return: 1-4 => sequence length
	constexpr uint64_t utf8_seq_len(char32_t ch);
	// return: 0 => invalid code unit, 1-4 => sequence length
	constexpr uint64_t utf8_adjust_index_to_head(const char8_t* seq, uint64_t size, uint64_t index, uint64_t& adjusted_index);
	// return: index in code point
	constexpr uint64_t utf8_code_point_index(const char8_t* seq, uint32_t size, uint64_t index);
	// return: index in code unit
	constexpr uint64_t utf8_code_unit_index(const char8_t* seq, uint32_t size, uint64_t index);

	//==================> utf-16 <==================
	// is ch a leading surrogate
	constexpr bool utf16_is_leading_surrogate(char16_t ch);
	// is ch a trailing surrogate
	constexpr bool utf16_is_trailing_surrogate(char16_t ch);
	// is ch a surrogate
	constexpr bool utf16_is_surrogate(char16_t ch);
	// return: 0 => ch is trailing, 1-2 => sequence length (if ch is leading)
	constexpr uint64_t utf16_seq_len(char8_t ch);
	// return: 1-2 => sequence length
	constexpr uint64_t utf16_seq_len(char16_t ch);
	// return: 1-2 => sequence length
	constexpr uint64_t utf16_seq_len(char32_t ch);
	// return: 0 => invalid code unit, 1-2 => sequence length
	constexpr uint64_t utf16_adjust_index_to_head(const char16_t* seq, uint64_t size, uint64_t index, uint64_t& adjusted_index);
	// return: index in code point
	constexpr uint64_t utf16_code_point_index(const char16_t* seq, uint32_t size, uint64_t index);
	// return: index in code unit
	constexpr uint64_t utf16_code_unit_index(const char16_t* seq, uint32_t size, uint64_t index);

	//==================> sequence <==================
	struct UTF8Seq;
	struct UTF16Seq;

	struct UTF8Seq {
		// ctor
		constexpr UTF8Seq();
		constexpr UTF8Seq(const char8_t* c_str, uint8_t len);
		constexpr UTF8Seq(char8_t ch_0);
		constexpr UTF8Seq(char8_t ch_0, char8_t ch_1);
		constexpr UTF8Seq(char8_t ch_0, char8_t ch_1, char8_t ch_2);
		constexpr UTF8Seq(char8_t ch_0, char8_t ch_1, char8_t ch_2, char8_t ch_3);

		// cast
		constexpr UTF8Seq(const UTF16Seq& seq);
		constexpr UTF8Seq(char32_t ch);
		constexpr operator char32_t() const;

		// factory
		constexpr static UTF8Seq Bad(char8_t bad_ch);
		constexpr static UTF8Seq ParseUTF8(const char8_t* seq, uint64_t size, uint64_t index, uint64_t& adjusted_index);
		constexpr static UTF8Seq ParseUTF16(const char16_t* seq, uint64_t size, uint64_t index, uint64_t& adjusted_index);

		// compare
		constexpr bool operator==(const UTF8Seq& rhs) const;

		// validate
		constexpr bool is_valid() const;
		constexpr operator bool() const;

		// visitor
		constexpr char8_t& at(uint32_t index);
		constexpr char8_t at(uint32_t index) const;
		constexpr char8_t& operator[](uint32_t index);
		constexpr char8_t operator[](uint32_t index) const;

		// convert len
		constexpr uint64_t to_utf16_len() const;

		alignas(4) char8_t data[4] = {};
		char8_t bad_data = {};
		uint8_t len = 0;
	};

	struct UTF16Seq {
		// ctor
		constexpr UTF16Seq();
		constexpr UTF16Seq(const char16_t* c_str, uint8_t len);
		constexpr UTF16Seq(char16_t ch_0);
		constexpr UTF16Seq(char16_t ch_0, char16_t ch_1);

		// cast
		constexpr UTF16Seq(const UTF8Seq& seq);
		constexpr UTF16Seq(char32_t ch);
		constexpr operator char32_t() const;

		// factory
		constexpr static UTF16Seq Bad(char16_t bad_ch);
		constexpr static UTF16Seq ParseUTF8(const char8_t* seq, uint64_t size, uint64_t index, uint64_t& adjusted_index);
		constexpr static UTF16Seq ParseUTF16(const char16_t* seq, uint64_t size, uint64_t index, uint64_t& adjusted_index);

		// compare
		constexpr bool operator==(const UTF16Seq& rhs) const;

		// validate
		constexpr bool is_valid() const;
		constexpr operator bool() const;

		// visitor
		constexpr char16_t& at(uint32_t index);
		constexpr char16_t at(uint32_t index) const;
		constexpr char16_t& operator[](uint32_t index);
		constexpr char16_t operator[](uint32_t index) const;

		// convert len
		constexpr uint64_t to_utf8_len() const;

		alignas(4) char16_t data[2] = {};
		char16_t bad_data = {};
		uint8_t len = 0;
	};
}

namespace u8lib
{
	//==================> utf-8 <==================
	constexpr char32_t utf8_maximum_code_point(uint64_t seq_len) {
		constexpr std::array<char32_t, 4> maximum_codepoints{
			static_cast<char32_t>(0x0000'007F),
			static_cast<char32_t>(0x0000'07FF),
			static_cast<char32_t>(0x0000'FFFF),
			static_cast<char32_t>(0x001F'FFFF),
		};
		return maximum_codepoints.at(seq_len - 1);
	}

	constexpr char8_t utf8_mask(uint64_t seq_len) {
		constexpr std::array<char8_t, 5> masks{
			static_cast<char8_t>(0b0011'1111),
			static_cast<char8_t>(0b0111'1111),
			static_cast<char8_t>(0b0001'1111),
			static_cast<char8_t>(0b0000'1111),
			static_cast<char8_t>(0b0000'0111),
		};
		return masks.at(seq_len);
	}

	constexpr uint64_t utf8_seq_len(char8_t ch) {
		if (ch == 0) {
			return 1;
		}

		constexpr char8_t mask = static_cast<char8_t>(0b1000'0000);
		uint32_t size = 0;
		uint8_t v = ch;
		while (v & mask) {
			++size;
			v <<= 1;
		}

		// 10xx'xxxx => trailing
		// 0xxx'xxxx => 1-byte
		// 110x'xxxx => 2-byte
		// 1110'xxxx => 3-byte
		// 1111'0xxx => 4-byte
		return size > 1 ? size : 1 - size;
	}

	constexpr uint64_t utf8_seq_len(char16_t ch) {
		if (ch <= utf8_maximum_code_point(1)) {
			// 1-byte BPM codepoint, U+0000-U+007F
			return 1;
		}
		if (ch <= utf8_maximum_code_point(2)) {
			// 2-byte BPM codepoint, U+0080-U+07FF
			return 2;
		}
		if (utf16_is_surrogate(ch)) {
			// surrogate, means 4-byte SPM codepoint, U+D800-U+DFFF
			return 4;
		}

		// 3-byte BPM codepoint, U+0800-U+FFFF
		return 3;
	}

	constexpr uint64_t utf8_seq_len(char32_t ch) {
		if (ch <= utf8_maximum_code_point(1)) {
			return 1;
		}
		if (ch <= utf8_maximum_code_point(2)) {
			return 2;
		}
		if (ch <= utf8_maximum_code_point(3)) {
			return 3;
		}
		return 4;
	}

	constexpr uint64_t utf8_adjust_index_to_head(const char8_t* seq, uint64_t size, uint64_t index, uint64_t& adjusted_index) {
		uint64_t seq_len = utf8_seq_len(seq[index]);

		if (seq_len == 0) {
			if (index == 0) {
				// bad head
				adjusted_index = index;
				return 0;
			}
			// adjust to head
			uint64_t find_idx = index;
			uint64_t code_uint_count = 0;
			while (find_idx > 0) {
				--find_idx;
				++code_uint_count;
				seq_len = utf8_seq_len(seq[find_idx]);
				if (seq_len) {
					++code_uint_count;
					break;
				}
			}

			if (seq_len == 0) {
				// reach seq start, bad head
				adjusted_index = index;
				return 0;
			}
			if (seq_len < code_uint_count) {
				// count miss match
				adjusted_index = index;
				return 0;
			}

			// good head
			if (find_idx + seq_len > size) {
				// seq len overflow
				adjusted_index = index;
				return 0;
			}

			adjusted_index = find_idx;
			return seq_len;
		}

		adjusted_index = index;
		return index + seq_len <= size ? seq_len : 0; // avoid overflow
	}

	constexpr uint64_t utf8_code_point_index(const char8_t* seq, uint32_t size, uint64_t index) {
		assert(index < size);

		uint64_t cur_idx = 0;
		uint64_t code_point_count = 0;

		do {
			const auto seq_len = utf8_seq_len(seq[cur_idx]);
			cur_idx += seq_len
							? (cur_idx + seq_len <= size)
								? seq_len
								: 1
							:  // check bad ch in tail
							1; // invalid code unit
			++code_point_count;
		} while (cur_idx <= index);

		return code_point_count - 1;
	}

	constexpr uint64_t utf8_code_unit_index(const char8_t* seq, uint32_t size, uint64_t index) {
		assert(index < size);

		uint64_t cur_idx = 0;
		uint64_t code_point_count = 0;
		while (cur_idx < size && code_point_count < index) {
			const auto seq_len = utf8_seq_len(seq[cur_idx]);
			cur_idx += seq_len
							? (cur_idx + seq_len <= size)
								? seq_len
								: 1
							:  // check bad ch in tail
							1; // invalid code unit
			++code_point_count;
		}
		assert(code_point_count == index && "invalid code point index");
		return cur_idx;
	}

	//==================> utf-16 <==================
	constexpr bool utf16_is_leading_surrogate(char16_t ch) {
		return ch >= kUtf16LeadingSurrogateMin && ch <= kUtf16LeadingSurrogateMax;
	}

	constexpr bool utf16_is_trailing_surrogate(char16_t ch) {
		return ch >= kUtf16TrailingSurrogateMin && ch <= kUtf16TrailingSurrogateMax;
	}

	constexpr bool utf16_is_surrogate(char16_t ch) {
		return utf16_is_leading_surrogate(ch) || utf16_is_trailing_surrogate(ch);
	}

	constexpr uint64_t utf16_seq_len(char8_t ch) {
		// 0 => utf-8 trailing unit
		// 1-3 => BPM plane code point
		// 4 => SPM plane code point
		uint64_t utf8_len = utf8_seq_len(ch);
		return utf8_len == 0 ? 0 : utf8_len <= 3 ? 1 : 2;
	}

	constexpr uint64_t utf16_seq_len(char16_t ch) {
		return utf16_is_leading_surrogate(ch) ? 2 : utf16_is_trailing_surrogate(ch) ? 0 : 1;
	}

	constexpr uint64_t utf16_seq_len(char32_t ch) {
		return ch <= kBMPMaxCodePoint ? 1 : 2;
	}

	constexpr uint64_t utf16_adjust_index_to_head(const char16_t* seq, uint64_t size, uint64_t index, uint64_t& adjusted_index) {
		uint64_t seq_len = utf16_seq_len(seq[index]);

		if (seq_len == 0) {
			if (index == 0) {
				// bad head
				adjusted_index = index;
				return 0;
			}

			// adjust to head
			uint64_t find_idx = index - 1;
			seq_len = utf16_seq_len(seq[find_idx]);
			if (seq_len == 2) {
				// good code seq
				adjusted_index = find_idx;
				return seq_len;
			}

			// invalid code seq
			adjusted_index = index;
			return 0;
		}

		adjusted_index = index;
		return index + seq_len <= size ? seq_len : 0; // avoid overflow
	}

	constexpr uint64_t utf16_code_point_index(const char16_t* seq, uint32_t size, uint64_t index) {
		assert(index < size);

		uint64_t cur_idx = 0;
		uint64_t code_point_count = 0;
		do {
			const auto seq_len = utf16_seq_len(seq[cur_idx]);
			cur_idx += seq_len
							? (cur_idx + seq_len <= size)
								? seq_len
								: 1
							:  // check bad ch in tail
							1; // invalid code unit
			++code_point_count;
		} while (cur_idx <= index);
		return code_point_count - 1;
	}

	constexpr uint64_t utf16_code_unit_index(const char16_t* seq, uint32_t size, uint64_t index) {
		assert(index < size);

		uint64_t cur_idx = 0;
		uint64_t code_point_count = 0;
		while (cur_idx < size && code_point_count < index) {
			const auto seq_len = utf16_seq_len(seq[cur_idx]);
			cur_idx += seq_len
							? (cur_idx + seq_len <= size)
								? seq_len
								: 1
							:  // check bad ch in tail
							1; // invalid code unit
			++code_point_count;
		}
		assert(code_point_count == index && "invalid code point index");
		return cur_idx;
	}

	//==================> utf-8 sequence <==================
	constexpr UTF8Seq::UTF8Seq() = default;

	constexpr UTF8Seq::UTF8Seq(const char8_t* c_str, uint8_t len): len(len) {
		for (uint32_t i = 0; i < len; ++i) {
			data[i] = c_str[i];
		}
	}

	constexpr UTF8Seq::UTF8Seq(char8_t ch_0): data{ch_0, 0, 0, 0}, len(1) {}

	constexpr UTF8Seq::UTF8Seq(char8_t ch_0, char8_t ch_1): data{ch_0, ch_1, 0, 0}, len(2) {}

	constexpr UTF8Seq::UTF8Seq(char8_t ch_0, char8_t ch_1, char8_t ch_2): data{ch_0, ch_1, ch_2, 0}, len(3) {}

	constexpr UTF8Seq::UTF8Seq(char8_t ch_0, char8_t ch_1, char8_t ch_2, char8_t ch_3): data{ch_0, ch_1, ch_2, ch_3}, len(4) {}

	constexpr UTF8Seq::UTF8Seq(const UTF16Seq& seq) {
		char32_t u32_ch = static_cast<char32_t>(seq);
		*this = u32_ch;
	}

	constexpr UTF8Seq::UTF8Seq(char32_t ch) {
		if (ch <= utf8_maximum_code_point(1)) {
			data[0] = static_cast<char8_t>(ch);
			len = 1;
		} else if (ch <= utf8_maximum_code_point(2)) {
			data[0] = static_cast<char8_t>((ch >> 6) | 0xc0);
			data[1] = static_cast<char8_t>((ch & utf8_mask(0)) | 0x80);
			len = 2;
		} else if (ch <= utf8_maximum_code_point(3)) {
			data[0] = static_cast<char8_t>((ch >> 12) | 0xe0);
			data[1] = static_cast<char8_t>(((ch >> 6) & utf8_mask(0)) | 0x80);
			data[2] = static_cast<char8_t>((ch & utf8_mask(0)) | 0x80);
			len = 3;
		} else {
			data[0] = static_cast<char8_t>((ch >> 18) | 0xf0);
			data[1] = static_cast<char8_t>(((ch >> 12) & utf8_mask(0)) | 0x80);
			data[2] = static_cast<char8_t>(((ch >> 6) & utf8_mask(0)) | 0x80);
			data[3] = static_cast<char8_t>((ch & utf8_mask(0)) | 0x80);
			len = 4;
		}
	}

	constexpr UTF8Seq::operator char32_t() const {
		if (is_valid()) {
			const auto lead_mask = utf8_mask(len);
			constexpr auto trailing_mask = utf8_mask(0);
			constexpr uint64_t trailing_bits = 6;

			const auto ch_0 = data[0];
			char32_t result = static_cast<char32_t>(ch_0 & lead_mask);
			for (uint32_t i = 1; i < len; ++i) {
				const auto ch_i = data[i];
				result <<= trailing_bits;
				result |= ch_i & trailing_mask;
			}
			return result;
		}

		return 0;
	}

	// factory
	constexpr UTF8Seq UTF8Seq::Bad(char8_t bad_ch) {
		UTF8Seq result{};
		result.bad_data = bad_ch;
		return result;
	}

	constexpr UTF8Seq UTF8Seq::ParseUTF8(const char8_t* seq, uint64_t size, uint64_t index, uint64_t& adjusted_index) {
		// bad input
		if (!seq) return {};
		if (index >= size) return {};

		// adjust seq index
		auto seq_len = utf8_adjust_index_to_head(seq, size, index, adjusted_index);

		return seq_len ? UTF8Seq{seq + adjusted_index, static_cast<uint8_t>(seq_len)} : UTF8Seq::Bad(seq[adjusted_index]);
	}

	constexpr UTF8Seq UTF8Seq::ParseUTF16(const char16_t* seq, uint64_t size, uint64_t index, uint64_t& adjusted_index) {
		return {UTF16Seq::ParseUTF16(seq, size, index, adjusted_index)};
	}

	// compare
	constexpr bool UTF8Seq::operator==(const UTF8Seq& rhs) const {
		if (len != rhs.len) return false;
		for (uint32_t i = 0; i < len; ++i) {
			if (data[i] != rhs.data[i]) return false;
		}
		return true;
	}

	// validate
	constexpr bool UTF8Seq::is_valid() const {
		return len > 0;
	}

	constexpr UTF8Seq::operator bool() const {
		return is_valid();
	}

	// visitor
	constexpr char8_t& UTF8Seq::at(uint32_t index) {
		return data[index];
	}

	constexpr char8_t UTF8Seq::at(uint32_t index) const {
		return data[index];
	}

	constexpr char8_t& UTF8Seq::operator[](uint32_t index) {
		return data[index];
	}

	constexpr char8_t UTF8Seq::operator[](uint32_t index) const {
		return data[index];
	}

	// convert len
	constexpr uint64_t UTF8Seq::to_utf16_len() const {
		return is_valid() ? utf16_seq_len(data[0]) : 1;
	}

	//==================> utf-16 sequence <==================
	constexpr UTF16Seq::UTF16Seq() = default;

	constexpr UTF16Seq::UTF16Seq(const char16_t* c_str, uint8_t len): len(len) {
		for (uint32_t i = 0; i < len; ++i) {
			data[i] = c_str[i];
		}
	}

	constexpr UTF16Seq::UTF16Seq(char16_t ch_0): data{ch_0, 0}, len(1) {}

	constexpr UTF16Seq::UTF16Seq(char16_t ch_0, char16_t ch_1): data{ch_0, ch_1}, len(2) {}

	// cast
	constexpr UTF16Seq::UTF16Seq(const UTF8Seq& seq) {
		char32_t u32_ch = static_cast<char32_t>(seq);
		*this = u32_ch;
	}

	constexpr UTF16Seq::UTF16Seq(char32_t ch) {
		if (ch <= kBMPMaxCodePoint) {
			data[0] = static_cast<char16_t>(ch);
			len = 1;
		} else {
			data[0] = static_cast<char16_t>(((ch - kSMPBaseCodePoint) >> 10) + kUtf16LeadingSurrogateHeader);
			data[1] = static_cast<char16_t>((ch & kUtf16SurrogateMask) + kUtf16TrailingSurrogateHeader);
			len = 2;
		}
	}

	constexpr UTF16Seq::operator char32_t() const {
		//                  1         0
		//         9876543210 9876543210
		//         |||||||||| ||||||||||
		// [110110]9876543210 |||||||||| high surrogate
		//            [110111]9876543210 low  surrogate
		return len == 1 ? data[0] : ((data[0] & kUtf16SurrogateMask) << 10) + (data[1] & kUtf16SurrogateMask) + kSMPBaseCodePoint;
	}

	// factory
	constexpr UTF16Seq UTF16Seq::Bad(char16_t bad_ch) {
		UTF16Seq result{};
		result.bad_data = bad_ch;
		return result;
	}

	constexpr UTF16Seq UTF16Seq::ParseUTF8(const char8_t* seq, uint64_t size, uint64_t index, uint64_t& adjusted_index) {
		return {UTF8Seq::ParseUTF8(seq, size, index, adjusted_index)};
	}

	constexpr UTF16Seq UTF16Seq::ParseUTF16(const char16_t* seq, uint64_t size, uint64_t index, uint64_t& adjusted_index) {
		// bad input
		if (!seq) return {};
		if (index >= size) return {};

		// adjust seq index
		auto seq_len = utf16_adjust_index_to_head(seq, size, index, adjusted_index);

		return seq_len ? UTF16Seq{seq + adjusted_index, static_cast<uint8_t>(seq_len)} : UTF16Seq::Bad(seq[adjusted_index]);
	}

	// compare
	constexpr bool UTF16Seq::operator==(const UTF16Seq& rhs) const {
		if (len != rhs.len) return false;
		for (uint32_t i = 0; i < len; ++i) {
			if (data[i] != rhs.data[i]) return false;
		}
		return true;
	}

	// validate
	constexpr bool UTF16Seq::is_valid() const {
		return len > 0;
	}

	constexpr UTF16Seq::operator bool() const {
		return is_valid();
	}

	// visitor
	constexpr char16_t& UTF16Seq::at(uint32_t index) {
		return data[index];
	}

	constexpr char16_t UTF16Seq::at(uint32_t index) const {
		return data[index];
	}

	constexpr char16_t& UTF16Seq::operator[](uint32_t index) {
		return data[index];
	}

	constexpr char16_t UTF16Seq::operator[](uint32_t index) const {
		return data[index];
	}

	// convert len
	constexpr uint64_t UTF16Seq::to_utf8_len() const {
		return is_valid() ? utf8_seq_len(data[0]) : 1;
	}
}
