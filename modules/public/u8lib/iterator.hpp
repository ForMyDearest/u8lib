#pragma once

#include "algorithm.hpp"
#include "container.hpp"
#include "decode_utf.hpp"

#include <string>
#include <iterator>

namespace u8lib
{
	class unicode_codepoint_iterator {
	public:
		using value_type = char32_t;
		using difference_type = ptrdiff_t;

		constexpr unicode_codepoint_iterator(const char8_t* first_val, const char8_t* last_val) noexcept : first(first_val), last(last_val) {
			next = decode_utf(first, last, val).next_ptr_;
		}

		constexpr unicode_codepoint_iterator() = default;

		constexpr unicode_codepoint_iterator& operator++() noexcept {
			first = next;
			if (first != last) {
				next = decode_utf(first, last, val).next_ptr_;
			}

			return *this;
		}

		constexpr unicode_codepoint_iterator operator++(int) noexcept {
			const auto old = *this;
			++*this;
			return old;
		}

		[[nodiscard]] constexpr value_type operator*() const noexcept {
			return val;
		}

		[[nodiscard]] constexpr const char8_t* position() const noexcept {
			return first;
		}

		[[nodiscard]] constexpr bool operator==(std::default_sentinel_t) const noexcept {
			return first == last;
		}

		[[nodiscard]] constexpr bool operator==(const unicode_codepoint_iterator& other) const noexcept {
			assert(last == other.last);
			return first == other.first && last == other.last;
		}

	private:
		const char8_t* first = nullptr;
		const char8_t* last = nullptr;
		const char8_t* next = nullptr;
		char32_t val = 0;
	};

	template<bool kConst>
	struct UTF8Cursor;
	template<bool kConst>
	using UTF8Iter = CursorIter<UTF8Cursor<kConst>, false>;
	template<bool kConst>
	using UTF8IterInv = CursorIter<UTF8Cursor<kConst>, true>;
	template<bool kConst>
	using UTF8Range = CursorRange<UTF8Cursor<kConst>, false>;
	template<bool kConst>
	using UTF8RangeInv = CursorRange<UTF8Cursor<kConst>, true>;

	template<bool kConst>
	struct UTF16Cursor;
	template<bool kConst>
	using UTF16Iter = CursorIter<UTF16Cursor<kConst>, false>;
	template<bool kConst>
	using UTF16IterInv = CursorIter<UTF16Cursor<kConst>, true>;
	template<bool kConst>
	using UTF16Range = CursorRange<UTF16Cursor<kConst>, false>;
	template<bool kConst>
	using UTF16RangeInv = CursorRange<UTF16Cursor<kConst>, true>;

	template<bool kConst>
	struct UTF8Cursor {
		using value_type = std::conditional_t<kConst, const char8_t, char8_t>;
		using size_type = size_t;
		using RefType = UTF8Seq;

		static constexpr size_type npos = static_cast<size_type>(-1);

		// iter & range
		using Iter = UTF8Iter<kConst>;
		using IterInv = UTF8IterInv<kConst>;
		using Range = UTF8Range<kConst>;
		using RangeInv = UTF8RangeInv<kConst>;

		// ctor & copy & move & assign & move assign
		constexpr UTF8Cursor(value_type* data, size_type size, size_type index)
			: _data(data), _size(size), _index(index) {
			_try_parse_seq_len();
		}

		constexpr UTF8Cursor(const UTF8Cursor& other) = default;
		constexpr UTF8Cursor(UTF8Cursor&& other) noexcept = default;
		constexpr UTF8Cursor& operator=(const UTF8Cursor& rhs) = default;
		constexpr UTF8Cursor& operator=(UTF8Cursor&& rhs) noexcept = default;

		// factory
		static constexpr UTF8Cursor Begin(value_type* data, size_type size) {
			return {data, size, 0};
		}

		static constexpr UTF8Cursor BeginOverflow(value_type* data, size_type size) {
			return {data, size, npos};
		}

		static constexpr UTF8Cursor End(value_type* data, size_type size) {
			UTF8Cursor result{data, size, npos};
			result.reset_to_end();
			return result;
		}

		static constexpr UTF8Cursor EndOverflow(value_type* data, size_type size) {
			return {data, size, size};
		}

		// getter
		constexpr RefType ref() const {
			return _seq_len == npos ? UTF8Seq::Bad(_data[_index]) : UTF8Seq{_data + _index, static_cast<uint8_t>(_seq_len)};
		}

		constexpr size_type index() const { return _index; }
		constexpr size_type seq_len() const { return _seq_len; }

		// move & reset
		constexpr void move_next() {
			assert(is_valid());
			_index += _seq_len == npos ? 1 : _seq_len; // _seq_len is always safe, because it has adjusted in _try_parse_seq_len
			_try_parse_seq_len();
		}

		constexpr void move_prev() {
			assert(is_valid());
			_seq_len = utf8_adjust_index_to_head(_data, _size, _index - 1, _index);
			_seq_len = _seq_len ? _seq_len : npos;
		}

		constexpr void reset_to_begin() {
			_index = 0;
			_try_parse_seq_len();
		}

		constexpr void reset_to_end() {
			_seq_len = u8lib::utf8_adjust_index_to_head(_data, _size, _size - 1, _index);
			_seq_len = _seq_len ? _seq_len : npos;
		}

		// reach & validate
		constexpr bool reach_end() const { return _index == _size; }
		constexpr bool reach_begin() const { return _index == npos; }
		constexpr bool is_valid() const { return !reach_end() && !reach_begin(); }

		// compare
		constexpr bool operator==(const UTF8Cursor& rhs) const { return _data == rhs._data && _index == rhs._index; }
		constexpr bool operator!=(const UTF8Cursor& rhs) const { return !(*this == rhs); }

		// convert
		constexpr Iter as_iter() const { return {*this}; }
		constexpr IterInv as_iter_inv() const { return {*this}; }
		constexpr Range as_range() const { return {*this}; }
		constexpr RangeInv as_range_inv() const { return {*this}; }

	private:
		constexpr void _try_parse_seq_len() {
			if (is_valid()) {
				_seq_len = utf8_seq_len(_data[_index]);
				// if the last sequence is invalid, set it to 1
				// this operation can avoid visit and move operation overflow
				// TODO. maybe 0 is better
				if (!_seq_len || _index + _seq_len > _size) {
					_seq_len = npos;
				}
			}
		}

		value_type* _data = nullptr;
		size_type _size = 0;
		size_type _index = 0;
		size_type _seq_len = 0; // npos means bad sequence
	};

	template<bool kConst>
	struct UTF16Cursor {
		using value_type = std::conditional_t<kConst, const char16_t, char16_t>;
		using size_type = size_t;
		using RefType = UTF16Seq;

		static constexpr size_type npos = static_cast<size_type>(-1);

		// iter & range
		using Iter = UTF16Iter<kConst>;
		using IterInv = UTF16IterInv<kConst>;
		using Range = UTF16Range<kConst>;
		using RangeInv = UTF16RangeInv<kConst>;

		// ctor & copy & move & assign & move assign
		constexpr UTF16Cursor(value_type* data, size_type size, size_type index)
			: _data(data), _size(size), _index(index) {
			_try_parse_seq_len();
		}

		constexpr UTF16Cursor(const UTF16Cursor& other) = default;
		constexpr UTF16Cursor(UTF16Cursor&& other) noexcept = default;
		constexpr UTF16Cursor& operator=(const UTF16Cursor& rhs) = default;
		constexpr UTF16Cursor& operator=(UTF16Cursor&& rhs) noexcept = default;

		// factory
		static constexpr UTF16Cursor Begin(value_type* data, size_type size) {
			return {data, size, 0};
		}

		static constexpr UTF16Cursor BeginOverflow(value_type* data, size_type size) {
			return {data, size, npos};
		}

		static constexpr UTF16Cursor End(value_type* data, size_type size) {
			UTF16Cursor result{data, size, npos};
			result.reset_to_end();
			return result;
		}

		static constexpr UTF16Cursor EndOverflow(value_type* data, size_type size) {
			return {data, size, size};
		}

		// getter
		constexpr RefType ref() const {
			return _seq_len == npos ? UTF16Seq::Bad(_data[_index]) : UTF16Seq{_data + _index, static_cast<uint8_t>(_seq_len)};
		}

		constexpr size_type index() const { return _index; }
		constexpr size_type seq_len() const { return _seq_len; }

		// move & reset
		constexpr void move_next() {
			assert(is_valid());
			_index += _seq_len == npos ? 1 : _seq_len; // _seq_len is always safe, because it has adjusted in _try_parse_seq_len
			_try_parse_seq_len();
		}

		constexpr void move_prev() {
			assert(is_valid());
			_seq_len = utf16_adjust_index_to_head(_data, _size, _index - 1, _index);
			_seq_len = _seq_len ? _seq_len : npos;
		}

		constexpr void reset_to_begin() {
			_index = 0;
			_try_parse_seq_len();
		}

		constexpr void reset_to_end() {
			_seq_len = u8lib::utf16_adjust_index_to_head(_data, _size, _size - 1, _index);
			_seq_len = _seq_len ? _seq_len : npos;
		}

		// reach & validate
		constexpr bool reach_end() const { return _index == _size; }
		constexpr bool reach_begin() const { return _index == npos; }
		constexpr bool is_valid() const { return !reach_end() && !reach_begin(); }

		// compare
		constexpr bool operator==(const UTF16Cursor& rhs) const { return _data == rhs._data && _index == rhs._index; }
		constexpr bool operator!=(const UTF16Cursor& rhs) const { return !(*this == rhs); }

		// convert
		constexpr Iter as_iter() const { return {*this}; }
		constexpr IterInv as_iter_inv() const { return {*this}; }
		constexpr Range as_range() const { return {*this}; }
		constexpr RangeInv as_range_inv() const { return {*this}; }

	private:
		constexpr void _try_parse_seq_len() {
			if (is_valid()) {
				_seq_len = u8lib::utf16_seq_len(_data[_index]);
				// if the last sequence is invalid, set it to 1
				// this operation can avoid visit and move operation overflow
				// TODO. maybe 0 is better
				if (!_seq_len || _index + _seq_len > _size) {
					_seq_len = npos;
				}
			}
		}

		value_type* _data = nullptr;
		size_type _size = 0;
		size_type _index = 0;
		size_type _seq_len = 0; // npos means bad sequence
	};

	//! @return byte size of unicode str
	template<is_char_v Char>
	constexpr size_t text_size(const Char* str, size_t len) {
		if constexpr (sizeof(Char) == sizeof(char8_t)) {
			return len;
		} else if constexpr (sizeof(Char) == sizeof(char16_t)) {
			// parse utf8 str len
			size_t utf8_len = 0;
			for (UTF16Seq utf16_seq: UTF16Cursor<true>{reinterpret_cast<const char16_t*>(str), len, 0}.as_range()) {
				if (utf16_seq.is_valid()) {
					utf8_len += utf16_seq.to_utf8_len();
				} else {
					utf8_len += 1;
				}
			}
			return utf8_len;
		} else if constexpr (sizeof(Char) == sizeof(char32_t)) {
			size_t utf8_len = 0;
			for (size_t i = 0; i < len; ++i) {
				utf8_len += utf8_seq_len(reinterpret_cast<const char32_t*>(str)[i]);
			}
			return utf8_len;
		} else {
			return 0;
		}
	}

	template<is_char_v Char>
	constexpr size_t text_size(const Char* str) {
		return text_length(str, std::char_traits<Char>::length(str));
	}

	template<is_char_v Char>
	constexpr void parse_to_utf8(const Char* str, size_t len, char8_t* dst) {
		if constexpr (sizeof(Char) == sizeof(char8_t)) {
			std::char_traits<char8_t>::copy(dst, reinterpret_cast<const char8_t*>(str), len);
		} else if constexpr (sizeof(Char) == sizeof(char16_t)) {
			size_t write_index = 0;
			for (UTF16Seq utf16_seq: UTF16Cursor<true>{reinterpret_cast<const char16_t*>(str), len, 0}.as_range()) {
				if (utf16_seq.is_valid()) {
					UTF8Seq utf8_seq = utf16_seq;
					std::char_traits<char8_t>::copy(dst + write_index, utf8_seq.data, utf8_seq.len);
					write_index += utf8_seq.len;
				} else {
					std::char_traits<char8_t>::copy(dst + write_index, reinterpret_cast<char8_t*>(&utf16_seq.bad_data), 1);
					++write_index;
				}
			}
		} else if constexpr (sizeof(Char) == sizeof(char32_t)) {
			size_t write_index = 0;
			for (size_t i = 0; i < len; ++i) {
				UTF8Seq utf8_seq = reinterpret_cast<const char32_t*>(str)[i];
				std::char_traits<char8_t>::copy(dst + write_index, utf8_seq.data, utf8_seq.len);
				write_index += utf8_seq.len;
			}
		}
	}
}
