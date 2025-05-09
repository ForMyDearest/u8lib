#pragma once

//! @bug 2025/03/14  MSVC, 位域下 copy move 等操作会影响 sso_size，原因不明

#include <memory>

#include "auxiliary/unicode/u8string.hpp"

// internal
namespace auxiliary::unicode
{
	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr void U8String<Traits, Allocator, ContainerPolicy, SSOSize>::_reset() noexcept {
		std::memset(buffer_, 0, SSOBufferSize);
		sso_flag_ = 1;
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr void U8String<Traits, Allocator, ContainerPolicy, SSOSize>::_set_size(size_type value) noexcept {
		if (is_sso()) {
			sso_data_[value] = 0;
			sso_size_ = value;
		}
		else {
			data_[value] = 0;
			size_ = value;
		}
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	template<typename Getter, typename Fn> constexpr
	void U8String<Traits, Allocator, ContainerPolicy, SSOSize>::_reserve(Getter&& getter, size_type count, Fn&& func) {
		if (count > capacity()) {
			auto new_sz = std::forward<Getter>(getter)(count + 1);
			pointer new_memory = allocator_type::allocate(new_sz);
			std::forward<Fn>(func)(new_memory);

			if (is_heap()) {
				allocator_type::deallocate(data(), capacity_ + 1);
			}

			data_ = new_memory;
			capacity_ = new_sz - 1;
			sso_flag_ = 0;
		}
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	template<typename Fn>
	constexpr void U8String<Traits, Allocator, ContainerPolicy, SSOSize>::_insert(size_type index, size_type len, Fn&& func) {
		auto at_least_capacity = size() + len;
		if (at_least_capacity > capacity()) {
			auto new_sz = policy_type::get_grow(at_least_capacity + 1);
			pointer new_memory = allocator_type::allocate(new_sz);
			traits_type::move(new_memory, data(), index);
			traits_type::move(new_memory + index + len, data() + index, size() - index + 1);
			std::forward<Fn>(func)(new_memory + index);

			if (is_heap()) {
				allocator_type::deallocate(data(), capacity_ + 1);
			}

			data_ = new_memory;
			capacity_ = new_sz - 1;
			sso_flag_ = 0;
		}
		else {
			traits_type::move(data() + index + len, data() + index, size() - index + 1);
			std::forward<Fn>(func)(data() + index);
		}
		_set_size(at_least_capacity);
	}
}

// join
namespace auxiliary::unicode
{
	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	template<typename... Args>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::concat(Args&&... string_or_view) {
		std::array<view_type, sizeof...(Args)> args{view_type{string_or_view}...};

		// calc size
		size_type total_size = 0;
		for (const auto& arg: args) {
			total_size += arg.size();
		}

		// combine
		U8String result;
		result.reserve(total_size);
		for (const auto& arg: args) {
			result.append(arg);
		}

		return result;
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	template<typename Container>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::join(const Container& container, view_type separator, bool skip_empty, view_type trim_chs) {
		// calc size
		size_type total_size = 0;
		bool is_first_append = false;
		for (const auto& str: container) {
			view_type view{str};

			// trim
			if (!trim_chs.empty()) {
				view = view.trim(trim_chs);
			}

			// skip empty
			if (skip_empty && view.empty()) continue;

			// append separator
			if (!is_first_append) {
				is_first_append = true;
			}
			else {
				total_size += separator.size();
			}

			// append item
			total_size += view.size();
		}

		// combine
		U8String result;
		result.reserve(total_size);
		is_first_append = false;
		for (const auto& str: container) {
			view_type view{str};

			// trim
			if (!trim_chs.empty()) {
				view = view.trim(trim_chs);
			}

			// skip empty
			if (skip_empty && view.empty()) continue;

			// append separator
			if (!is_first_append) {
				is_first_append = true;
			}
			else {
				result.append(separator);
			}

			// append item
			result.append(view);
		}

		AUXILIARY_ASSERT(result.size() == total_size && "Join failed");

		return result;
	}
}

// ctor & dtor
namespace auxiliary::unicode
{
	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>::U8String() {
		_reset();
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>::U8String(size_type count, value_type ch) {
		_reset();
		_reserve(policy_type::get_reserve, count, [](pointer) {});
		std::uninitialized_fill_n(data(), count, ch);
		_set_size(count);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	template<is_char_v Char>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>::U8String(const Char* str)
		: U8String(str, std::char_traits<Char>::length(str)) {}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	template<is_char_v Char>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>::U8String(const Char* str, size_type len) {
		_reset();
		assign(str, len);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	template<StrViewLike<false> View>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>::U8String(View view)
		: U8String(view.data(), view.size()) {}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	template<StrViewLike<false> View>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>::U8String(View view, size_type pos)
		: U8String(view.data() + pos, view.size() - pos) {}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	template<StrViewLike<false> View>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>::U8String(View view, size_type pos, size_type count)
		: U8String(view.data() + pos, count) {}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	template<StrViewLike<true> Str>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>::U8String(const Str& str)
		: U8String(str.data(), str.size()) {}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	template<StrViewLike<true> Str>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>::U8String(const Str& str, size_type pos)
		: U8String(str.data() + pos, str.size() - pos) {}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	template<StrViewLike<true> Str>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>::U8String(const Str& str, size_type pos, size_type count)
		: U8String(str.data() + pos, count) {}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>::U8String(const U8String& rhs)
		: U8String(rhs.view()) {}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>::U8String(U8String&& rhs) noexcept {
		_reset();
		swap(rhs);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	template<typename A> constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>::U8String(A&& a)
		requires (std::is_constructible_v<Allocator, A>) : Allocator(std::forward<A>(a)) {
		_reset();
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	template<typename A, is_char_v Char>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>::U8String(const Char* str, A&& a)
		requires (std::is_constructible_v<Allocator, A>)
		: U8String(str, std::char_traits<Char>::length(str), std::forward<A>(a)) {}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	template<typename A, is_char_v Char>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>::U8String(const Char* str, size_type len, A&& a)
		requires (std::is_constructible_v<Allocator, A>): Allocator(std::forward<A>(a)) {
		_reset();
		assign(str, len);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	template<typename A>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>::U8String(size_type count, value_type ch, A&& a)
		requires (std::is_constructible_v<Allocator, A>): Allocator(std::forward<A>(a)) {
		_reset();
		_reserve(policy_type::get_reserve, count, [](pointer) {});
		std::uninitialized_fill_n(data(), count, ch);
		_set_size(count);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	template<typename A, StrViewLike<false> View>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>::U8String(View view, A&& a)
		requires (std::is_constructible_v<Allocator, A>)
		: U8String(view.data(), view.size(), std::forward<A>(a)) {}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	template<typename A, StrViewLike<false> View>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>::U8String(View view, size_type pos, A&& a)
		requires (std::is_constructible_v<Allocator, A>)
		: U8String(view.data() + pos, view.size() - pos, std::forward<A>(a)) {}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	template<typename A, StrViewLike<false> View>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>::U8String(View view, size_type pos, size_type count, A&& a)
		requires (std::is_constructible_v<Allocator, A>)
		: U8String(view.data() + pos, count, std::forward<A>(a)) {}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	template<typename A, StrViewLike<true> Str>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>::U8String(const Str& str, A&& a)
		requires (std::is_constructible_v<Allocator, A>)
		: U8String(str.data(), str.size(), std::forward<A>(a)) {}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	template<typename A, StrViewLike<true> Str>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>::U8String(const Str& str, size_type pos, A&& a)
		requires (std::is_constructible_v<Allocator, A>)
		: U8String(str.data() + pos, str.size() - pos, std::forward<A>(a)) {}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	template<typename A, StrViewLike<true> Str>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>::U8String(const Str& str, size_type pos, size_type count, A&& a)
		requires (std::is_constructible_v<Allocator, A>)
		: U8String(str.data() + pos, count, std::forward<A>(a)) {}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	template<typename A>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>::U8String(U8String&& rhs, A&& a)
		requires (std::is_constructible_v<Allocator, A>) : Allocator(std::forward<A>(a)) {
		_reset();
		swap(rhs);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>::~U8String() noexcept {
		if (is_heap()) allocator_type::deallocate(data_, capacity_ + 1);
	}
}

// assign
namespace auxiliary::unicode
{
	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>&
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::operator=(const U8String& rhs) {
		assign(rhs);
		return *this;
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>&
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::operator=(U8String&& rhs) noexcept {
		assign(std::move(rhs));
		return *this;
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	template<StrViewLike<false> View> constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>&
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::operator=(View view) {
		assign(view.data(), view.size());
		return *this;
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	template<StrViewLike<true> Str> constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>&
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::operator=(const Str& str) {
		assign(str.data(), str.size());
		return *this;
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	template<is_char_v Char> constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>&
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::operator=(const Char* str) {
		assign(str);
		return *this;
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>&
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::assign(U8String&& rhs) noexcept {
		if (is_heap()) allocator_type::deallocate(data_, capacity_ + 1);
		_reset();
		swap(rhs);
		return *this;
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	template<is_char_v Char>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>&
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::assign(const Char* str) {
		return assign(str, std::char_traits<Char>::length(str));
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	template<is_char_v Char>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>&
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::assign(const Char* str, size_type len) {
		size_type utf8_len = text_size(str, len);
		_reserve(policy_type::get_reserve, utf8_len, [](pointer) {});
		parse_to_utf8(str, len, data());
		_set_size(utf8_len);
		return *this;
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	template<StrViewLike<false> View>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>&
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::assign(View view) {
		return assign(view.data(), view.size());
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	template<StrViewLike<true> Str>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>&
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::assign(const Str& str) {
		return assign(str.data(), str.size());
	}
}

// compare
namespace auxiliary::unicode
{
	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr bool U8String<Traits, Allocator, ContainerPolicy, SSOSize>::operator==(view_type str) const noexcept {
		return view() == str;
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr bool U8String<Traits, Allocator, ContainerPolicy, SSOSize>::operator==(const_pointer str) const noexcept {
		return view() == view_type(str);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr bool U8String<Traits, Allocator, ContainerPolicy, SSOSize>::operator==(const U8String& str) const noexcept {
		return view() == str.view();
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr std::strong_ordering U8String<Traits, Allocator, ContainerPolicy, SSOSize>::operator<=>(const U8String& str) const noexcept {
		return view() <=> str.view();
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr std::strong_ordering U8String<Traits, Allocator, ContainerPolicy, SSOSize>::operator<=>(view_type str) const noexcept {
		return view() <=> str;
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr std::strong_ordering U8String<Traits, Allocator, ContainerPolicy, SSOSize>::operator<=>(const_pointer str) const noexcept {
		return view() <=> view_type(str);
	}


	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr int U8String<Traits, Allocator, ContainerPolicy, SSOSize>::compare(view_type v) const noexcept {
		return view().compare(v);
	}
}

// iterator
namespace auxiliary::unicode
{
	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::pointer
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::begin() noexcept {
		return data();
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::const_pointer
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::begin() const noexcept {
		return data();
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::const_pointer
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::cbegin() const noexcept {
		return data();
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::pointer
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::end() noexcept {
		return data() + size();
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::const_pointer
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::end() const noexcept {
		return data() + size();
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::const_pointer
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::cend() const noexcept {
		return data() + size();
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr std::reverse_iterator<typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::pointer>
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::rbegin() noexcept {
		return std::reverse_iterator(end());
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr std::reverse_iterator<typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::const_pointer>
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::rbegin() const noexcept {
		return std::reverse_iterator(end());
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr std::reverse_iterator<typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::const_pointer>
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::crbegin() const noexcept {
		return std::reverse_iterator(end());
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr std::reverse_iterator<typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::pointer>
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::rend() noexcept {
		return std::reverse_iterator(begin());
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr std::reverse_iterator<typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::const_pointer>
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::rend() const noexcept {
		return std::reverse_iterator(begin());
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr std::reverse_iterator<typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::const_pointer>
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::crend() const noexcept {
		return std::reverse_iterator(begin());
	}
}

// size
namespace auxiliary::unicode
{
	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr bool U8String<Traits, Allocator, ContainerPolicy, SSOSize>::empty() const noexcept {
		return !size();
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::size_type
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::length() const noexcept {
		return size();
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::size_type
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::length_text() const noexcept {
		return view().length_text();
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::size_type
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::capacity() const noexcept {
		return is_sso() ? SSOCapacity : capacity_;
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::size_type
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::slack() const noexcept {
		return capacity() - size();
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::size_type
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::size() const noexcept {
		return is_sso() ? sso_size_ : size_;
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::size_type
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::max_size() const noexcept {
		return view().max_size();
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	template<is_char_v Char>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::size_type
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::to_size() const noexcept {
		return view().template to_size<Char>();
	}
}

// data access
namespace auxiliary::unicode
{
	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::reference
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::at(size_type pos) {
		AUXILIARY_ASSERT(is_valid_index(pos));
		return data()[pos];
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::reference
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::last_buffer(size_type pos) {
		return at(size() - pos - 1);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::reference
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::operator[](size_type pos) {
		return at(pos);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::reference
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::front() {
		return at(0);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::reference
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::back() {
		return at(size() - 1);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::pointer
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::data() noexcept {
		return is_sso() ? sso_data_ : data_;
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr UTF8Seq U8String<Traits, Allocator, ContainerPolicy, SSOSize>::at_text(size_type index) const {
		return view().at_text(index);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr UTF8Seq U8String<Traits, Allocator, ContainerPolicy, SSOSize>::last_text(size_type index) const {
		return view().last_text(index);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::allocator_type
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::get_allocator() const noexcept {
		return *this;
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::const_reference
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::at(size_type pos) const {
		AUXILIARY_ASSERT(is_valid_index(pos));
		return data()[pos];
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::const_reference
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::last_buffer(size_type pos) const {
		return at(size() - pos - 1);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::const_reference
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::operator[](size_type pos) const {
		return at(pos);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::const_reference
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::front() const {
		return at(0);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::const_reference
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::back() const {
		return at(size() - 1);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::const_pointer
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::data() const noexcept {
		return is_sso() ? sso_data_ : data_;
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr const char* U8String<Traits, Allocator, ContainerPolicy, SSOSize>::raw_data() const noexcept {
		return reinterpret_cast<const char*>(data());
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::const_pointer
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::c_str() const noexcept {
		return data();
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr bool U8String<Traits, Allocator, ContainerPolicy, SSOSize>::is_sso() const noexcept {
		return sso_flag_;
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr bool U8String<Traits, Allocator, ContainerPolicy, SSOSize>::is_heap() const noexcept {
		return !sso_flag_;
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr bool U8String<Traits, Allocator, ContainerPolicy, SSOSize>::is_valid_index(size_type index) const noexcept {
		return index < size();
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::size_type
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::buffer_index_to_text(size_type index) const noexcept {
		return view().buffer_index_to_text(index);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::size_type
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::text_index_to_buffer(size_type index) const noexcept {
		return view().text_index_to_buffer(index);
	}
}

// sub view
namespace auxiliary::unicode
{
	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>::operator view_type() const noexcept {
		return view();
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::view_type
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::view() const noexcept {
		return view_type{data(), size()};
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::view_type
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::first_view(size_type count) const {
		return view().first(count);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::view_type
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::last_view(size_type count) const {
		return view().last(count);
	}


	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::view_type
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::subview(size_type start, size_type count) const noexcept {
		return view().substr(start, count);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::first(size_type count) const {
		return view().first(count);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::last(size_type count) const {
		return view().last(count);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::substr(size_type pos, size_type count) const {
		return view().substr(pos, count);
	}
}

// modify
namespace auxiliary::unicode
{
	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr void U8String<Traits, Allocator, ContainerPolicy, SSOSize>::reserve(size_type new_cap) {
		_reserve(policy_type::get_reserve, new_cap, [&](pointer ptr) {
			// '\0'
			traits_type::move(ptr, data(), size() + 1);
		});
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr void U8String<Traits, Allocator, ContainerPolicy, SSOSize>::shrink_to_fit() {}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr void U8String<Traits, Allocator, ContainerPolicy, SSOSize>::release(size_type reserve_capacity) {
		reserve(reserve_capacity);
		clear();
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr void U8String<Traits, Allocator, ContainerPolicy, SSOSize>::resize(size_type count) {
		const auto sz = size();
		reserve(count);
		if (sz < count) {
			std::uninitialized_default_construct_n(data() + sz, count);
		}
		_set_size(count);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr void U8String<Traits, Allocator, ContainerPolicy, SSOSize>::resize(size_type count, value_type ch) {
		const auto sz = size();
		reserve(count);
		if (sz < count) {
			std::uninitialized_fill_n(data() + sz, count, ch);
		}
		_set_size(count);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr void U8String<Traits, Allocator, ContainerPolicy, SSOSize>::swap(U8String& other) noexcept {
		std::swap(buffer_, other.buffer_);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>&
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::reverse(size_type start, size_type count) {
		AUXILIARY_ASSERT(is_valid_index(start) && "undefined behaviour accessing out of bounds");
		AUXILIARY_ASSERT(count == npos || count <= size() - start && "undefined behaviour exceeding size of string view");
		count = count == npos ? size() - start : count;

		std::reverse(data() + start, data() + start + count);
		return *this;
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::size_type
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::copy(pointer dest, size_type count, size_type pos) const {
		return view().copy(dest, count, pos);
	}
}

// add
namespace auxiliary::unicode
{
	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>&
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::insert(size_type index, size_type count, value_type ch) {
		AUXILIARY_ASSERT(index <= size());

		_insert(index, count, [&](pointer ptr) {
			std::uninitialized_fill_n(ptr, count, ch);
		});
		return *this;
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>&
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::insert(size_type index, UTF8Seq seq) {
		AUXILIARY_ASSERT(index <= size());
		if (seq.is_valid()) {
			return insert(index, seq.data, seq.len);
		}
		return *this;
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	template<is_char_v Char>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>&
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::insert(size_type index, const Char* s) {
		return insert(index, s, std::char_traits<Char>::length(s));
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	template<is_char_v Char>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>&
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::insert(size_type index, const Char* s, size_type len) {
		AUXILIARY_ASSERT(index <= size());

		auto utf8_len = text_size(s, len);
		_insert(index, utf8_len, [&](pointer ptr) {
			parse_to_utf8(s, len, ptr);
		});
		return *this;
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	template<StrViewLike<false> View>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>&
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::insert(size_type index, View view) {
		return insert(index, view.data(), view.size());
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	template<StrViewLike<true> Str>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>&
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::insert(size_type index, const Str& str) {
		return insert(index, str.data(), str.size());
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>&
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::push_back(value_type ch) {
		return append(1, ch);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>&
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::append(UTF8Seq seq) {
		if (seq.is_valid()) {
			return append(seq.data, seq.len);
		}
		return *this;
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>&
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::append(size_type count, value_type ch) {
		auto sz = size();
		size_type new_sz = sz + count;
		_reserve(policy_type::get_grow, new_sz, [&](pointer ptr) {
			traits_type::move(ptr, data(), sz);
		});
		std::uninitialized_fill_n(data() + sz, count, ch);
		_set_size(new_sz);
		return *this;
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	template<is_char_v Char>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>&
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::append(const Char* str) {
		return append(str, std::char_traits<Char>::length(str));
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	template<is_char_v Char>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>&
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::append(const Char* str, size_type len) {
		auto sz = size();
		size_type new_sz = sz + text_size(str, len);
		_reserve(policy_type::get_grow, new_sz, [&](pointer ptr) {
			traits_type::move(ptr, data(), sz);
		});
		parse_to_utf8(str, len, data() + sz);
		_set_size(new_sz);
		return *this;
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	template<StrViewLike<false> View>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>&
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::append(View view) {
		return append(view.data(), view.size());
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	template<StrViewLike<true> Str>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>&
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::append(const Str& str) {
		return append(str.data(), str.size());
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr void U8String<Traits, Allocator, ContainerPolicy, SSOSize>::operator+=(value_type ch) {
		push_back(ch);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr void U8String<Traits, Allocator, ContainerPolicy, SSOSize>::operator+=(UTF8Seq seq) {
		append(seq);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	template<is_char_v Char>
	constexpr void U8String<Traits, Allocator, ContainerPolicy, SSOSize>::operator+=(const Char* s) {
		append(s);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	template<StrViewLike<false> View>
	constexpr void U8String<Traits, Allocator, ContainerPolicy, SSOSize>::operator+=(View view) {
		append(view.data(), view.size());
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	template<StrViewLike<true> Str>
	constexpr void U8String<Traits, Allocator, ContainerPolicy, SSOSize>::operator+=(const Str& str) {
		append(str.data(), str.size());
	}
}

// remove
namespace auxiliary::unicode
{
	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>&
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::pop_back(size_type count) {
		AUXILIARY_ASSERT(size() >= count);
		_set_size(size() - count);
		return *this;
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>&
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::erase(size_type index, size_type count) {
		AUXILIARY_ASSERT(is_valid_index(index));

		auto v_end = std::min(count, size() - index);
		traits_type::move(data() + index, data() + index + v_end, size() - index - v_end);
		_set_size(size() - v_end);
		return *this;
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>&
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::clear() noexcept {
		_set_size(0);
		return *this;
	}
}

// replace
namespace auxiliary::unicode
{
	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>&
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::replace(size_type pos, size_type count, view_type view) {
		return replace(pos, count, view.data(), view.size());
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>&
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::replace(size_type pos, size_type count, const_pointer cstr, size_type count2) {
		AUXILIARY_ASSERT(pos + count <= size());

		auto at_least_capacity = size() + count2 - count;
		if (at_least_capacity > capacity()) {
			auto new_sz = policy_type::get_grow(at_least_capacity + 1);
			auto new_memory = allocator_type::allocate(new_sz);
			traits_type::move(new_memory, data(), pos);
			traits_type::copy(new_memory + pos, cstr, count2);
			traits_type::move(new_memory + pos + count2, data() + pos + count, size() - pos - count);

			if (is_heap()) {
				allocator_type::deallocate(data(), capacity_ + 1);
			}

			data_ = new_memory;
			capacity_ = new_sz - 1;
			sso_flag_ = 0;
		}
		else {
			traits_type::move(data() + pos + count2, data() + pos + count, size() - pos - count);
			traits_type::copy(data() + pos, cstr, count2);
		}

		_set_size(at_least_capacity);
		return *this;
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::Replace(size_type pos, size_type count, const_pointer cstr, size_type count2) const {
		return Replace(pos, count, view.data(), view.size());
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::Replace(size_type pos, size_type count, view_type view) const {
		return U8String(*this).replace(pos, count, view);
	}
}

// count & find
namespace auxiliary::unicode
{
	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr bool U8String<Traits, Allocator, ContainerPolicy, SSOSize>::starts_with(value_type ch) const noexcept {
		return view().starts_with(ch);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr bool U8String<Traits, Allocator, ContainerPolicy, SSOSize>::starts_with(view_type sv) const noexcept {
		return view().starts_with(sv);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr bool U8String<Traits, Allocator, ContainerPolicy, SSOSize>::starts_with(UTF8Seq seq) const noexcept {
		return view().starts_with(seq);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr bool U8String<Traits, Allocator, ContainerPolicy, SSOSize>::ends_with(value_type ch) const noexcept {
		return view().ends_with(ch);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr bool U8String<Traits, Allocator, ContainerPolicy, SSOSize>::ends_with(view_type sv) const noexcept {
		return view().ends_with(sv);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr bool U8String<Traits, Allocator, ContainerPolicy, SSOSize>::ends_with(UTF8Seq seq) const noexcept {
		return view().ends_with(seq);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr bool U8String<Traits, Allocator, ContainerPolicy, SSOSize>::contains(value_type ch) const noexcept {
		return view().contains(ch);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr bool U8String<Traits, Allocator, ContainerPolicy, SSOSize>::contains(UTF8Seq seq) const noexcept {
		return view().contains(seq);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::size_type
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::count(view_type pattern) const {
		return view().count(pattern);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::size_type
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::count(UTF8Seq seq) const {
		return view().count(seq);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr bool U8String<Traits, Allocator, ContainerPolicy, SSOSize>::contains(view_type sv) const noexcept {
		return view().contains(sv);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::size_type
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::find(const_pointer s, size_type pos, size_type count) const {
		return view().find(s, pos, count);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::size_type
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::find(value_type ch, size_type pos) const noexcept {
		return view().find(ch, pos);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::size_type
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::find(view_type v, size_type pos) const noexcept {
		return view().find(v, pos);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::size_type
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::find(UTF8Seq seq, size_type pos) const noexcept {
		return view().find(seq, pos);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::size_type
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::find_last(view_type pattern) const {
		return view().find_last(pattern);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::size_type
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::find_last(UTF8Seq seq) const {
		return view().find_last(seq);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::size_type
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::rfind(const_pointer s, size_type pos, size_type count) const {
		return view().rfind(s, pos, count);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::size_type
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::rfind(value_type ch, size_type pos) const noexcept {
		return view().rfind(ch, pos);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::size_type
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::rfind(view_type v, size_type pos) const noexcept {
		return view().rfind(v, pos);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::size_type
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::rfind(UTF8Seq seq, size_type pos) const noexcept {
		return view().rfind(seq, pos);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::size_type
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::find_first_of(const_pointer s, size_type pos, size_type count) const {
		return view().find_first_of(s, pos, count);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::size_type
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::find_first_of(value_type ch, size_type pos) const noexcept {
		return view().find_first_of(ch, pos);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::size_type
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::find_first_of(view_type v, size_type pos) const noexcept {
		return view().find_first_of(v, pos);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::size_type
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::find_first_of(UTF8Seq seq, size_type pos) const noexcept {
		return view().find_first_of(seq, pos);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::size_type
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::find_last_of(const_pointer s, size_type pos, size_type count) const {
		return view().find_last_of(s, pos, count);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::size_type
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::find_last_of(value_type ch, size_type pos) const noexcept {
		return view().find_last_of(ch, pos);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::size_type
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::find_last_of(view_type v, size_type pos) const noexcept {
		return view().find_last_of(v, pos);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::size_type
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::find_last_of(UTF8Seq seq, size_type pos) const noexcept {
		return view().find_last_of(seq, pos);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::size_type
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::find_first_not_of(const_pointer s, size_type pos, size_type count) const {
		return view().find_first_not_of(s, pos, count);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::size_type
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::find_first_not_of(value_type ch, size_type pos) const noexcept {
		return view().find_first_not_of(ch, pos);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::size_type
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::find_first_not_of(view_type v, size_type pos) const noexcept {
		return view().find_first_not_of(v, pos);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::size_type
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::find_first_not_of(UTF8Seq seq, size_type pos) const noexcept {
		return view().find_first_not_of(seq, pos);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::size_type
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::find_last_not_of(const_pointer s, size_type pos, size_type count) const {
		return view().find_last_not_of(s, pos, count);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::size_type
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::find_last_not_of(value_type ch, size_type pos) const noexcept {
		return view().find_last_not_of(ch, pos);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::size_type
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::find_last_not_of(view_type v, size_type pos) const noexcept {
		return view().find_last_not_of(v, pos);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::size_type
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::find_last_not_of(UTF8Seq seq, size_type pos) const noexcept {
		return view().find_last_not_of(seq, pos);
	}
}

// remove prefix & suffix
namespace auxiliary::unicode
{
	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>&
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::remove_prefix(view_type prefix) {
		if (starts_with(prefix)) {
			return erase(0, prefix.size());
		}
		return *this;
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>&
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::remove_prefix(UTF8Seq seq) {
		if (seq.is_valid()) {
			return remove_prefix(view_type(seq.data, seq.len));
		}
		return *this;
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>&
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::remove_suffix(view_type suffix) {
		if (ends_with(suffix)) {
			return erase(size() - suffix.size());
		}
		return *this;
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>&
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::remove_suffix(UTF8Seq seq) {
		if (seq.is_valid()) {
			return remove_suffix(view_type(seq.data, seq.len));
		}
		return *this;
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::RemovePrefix(view_type prefix) const {
		return view().RemovePrefix(prefix);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::RemovePrefix(UTF8Seq seq) const {
		return view().RemovePrefix(seq);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::RemoveSuffix(view_type suffix) const {
		return view().RemoveSuffix(suffix);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::RemoveSuffix(UTF8Seq seq) const {
		return view().RemoveSuffix(seq);
	}
}

// partition
namespace auxiliary::unicode
{
	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr std::array<typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::view_type, 3>
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::partition(view_type delimiter) const {
		return view().partition(delimiter);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr std::array<typename U8String<Traits, Allocator, ContainerPolicy, SSOSize>::view_type, 3>
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::partition(UTF8Seq delimiter) const {
		return view().partition(delimiter);
	}
}

// trim
namespace auxiliary::unicode
{
	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>&
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::trim(view_type characters) {
		return trim_start(characters).trim_end(characters);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>&
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::trim_start(view_type characters) {
		auto target = view().trim_start(characters);
		auto count = size() - target.size();
		return count ? erase(0, count) : *this;
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>&
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::trim_end(view_type characters) {
		auto target = view().trim_end(characters);
		return (target.size() == size()) ? *this : erase(target.size());
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>&
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::trim(UTF8Seq seq) {
		if (seq.is_valid()) {
			return trim(view_type(seq.data, seq.len));
		}
		return *this;
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>&
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::trim_start(UTF8Seq seq) {
		if (seq.is_valid()) {
			return trim_start(view_type(seq.data, seq.len));
		}
		return *this;
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>&
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::trim_end(UTF8Seq seq) {
		if (seq.is_valid()) {
			return trim_end(view_type(seq.data, seq.len));
		}
		return *this;
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>&
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::trim_invalid() {
		return trim_invalid_start().trim_invalid_end();
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>&
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::trim_invalid_start() {
		auto target = view().trim_invalid_start();
		auto count = size() - target.size();
		return count ? erase(0, count) : *this;
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>&
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::trim_invalid_end() {
		auto target = view().trim_invalid_end();
		return (target.size() == size()) ? *this : erase(target.size());
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::Trim(view_type characters) {
		return view().trim(characters);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::TrimStart(view_type characters) {
		return view().trim_start(characters);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::TrimEnd(view_type characters) {
		return view().trim_end(characters);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::Trim(UTF8Seq seq) {
		return view().trim(seq);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::TrimStart(UTF8Seq seq) {
		return view().trim_start(seq);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::TrimEnd(UTF8Seq seq) {
		return view().trim_end(seq);
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::TrimInvalid() {
		return view().trim_invalid();
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::TrimInvalidStart() {
		return view().trim_invalid_start();
	}

	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	constexpr U8String<Traits, Allocator, ContainerPolicy, SSOSize>
	U8String<Traits, Allocator, ContainerPolicy, SSOSize>::TrimInvalidEnd() {
		return view().trim_invalid_end();
	}
}
