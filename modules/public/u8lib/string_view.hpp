#pragma once

// TODO : support std::ranges operations

#include "base.hpp"
#include "iterator.hpp"

namespace u8lib
{
	/*!
	 * @note
	 *		1.Not support constexpr char operation \n
	 *		2.It's not a good idea to support implicit type conversion with std::basic_string_view
	 *		as  std::basic_string_view allows for implicit type conversion with cstring,
	 *		which can lead to \b ambiguity in the final construction
	 */
	class u8string_view {
	public:
		//==================> aligns <==================

		using value_type = char8_t;
		using pointer = value_type*;
		using const_pointer = const value_type*;
		using reference = value_type&;
		using const_reference = const value_type&;

		using raw_value_type = char;
		using raw_pointer = raw_value_type*;
		using raw_const_pointer = const raw_value_type*;
		using raw_reference = raw_value_type&;
		using raw_const_reference = const raw_value_type&;

		using size_type = size_t;
		using difference_type = ptrdiff_t;

		using const_data_reference = VectorDataRef<value_type, true>;
		using const_cursor = UTF8Cursor<true>;
		using const_iterator = UTF8Iter<true>;
		using const_reverse_iterator = UTF8IterInv<true>;
		using const_range = UTF8Range<true>;
		using const_reverse_range = UTF8RangeInv<true>;

		using traits_type = std::char_traits<value_type>;

		static constexpr size_type npos = const_data_reference::npos;

		//==================> ctor & dtor <==================

		constexpr u8string_view() noexcept;
		constexpr u8string_view(u8string_view&& other) noexcept = default;
		constexpr u8string_view(const u8string_view& other) noexcept = default;
		constexpr u8string_view(const_pointer str);
		constexpr u8string_view(const_pointer str, size_type count);
		u8string_view(raw_const_pointer str);
		u8string_view(raw_const_pointer str, size_type count);
		constexpr explicit u8string_view(std::nullptr_t) = delete;
		constexpr ~u8string_view() noexcept = default;

		//==================> assign <==================

		constexpr u8string_view& operator=(u8string_view&& rhs) noexcept = default;
		constexpr u8string_view& operator=(const u8string_view& rhs) noexcept = default;

		//==================> compare <==================

		constexpr bool operator==(const u8string_view& rhs) const noexcept;
		constexpr std::strong_ordering operator<=>(const u8string_view& rhs) const noexcept;

		constexpr int compare(u8string_view v) const noexcept;
		constexpr int compare(size_type pos1, size_type count1, u8string_view v) const;
		constexpr int compare(size_type pos1, size_type count1, u8string_view v, size_type pos2, size_type count2) const;
		constexpr int compare(size_type pos1, size_type count1, const_pointer s, size_type count2) const;

		//==================> iterator <==================

		constexpr const_pointer begin() const noexcept;
		constexpr const_pointer cbegin() const noexcept;
		constexpr const_pointer end() const noexcept;
		constexpr const_pointer cend() const noexcept;
		constexpr std::reverse_iterator<const_pointer> rbegin() const noexcept;
		constexpr std::reverse_iterator<const_pointer> crbegin() const noexcept;
		constexpr std::reverse_iterator<const_pointer> rend() const noexcept;
		constexpr std::reverse_iterator<const_pointer> crend() const noexcept;

		raw_const_pointer raw_begin() const noexcept;
		raw_const_pointer raw_cbegin() const noexcept;
		raw_const_pointer raw_end() const noexcept;
		raw_const_pointer raw_cend() const noexcept;
		std::reverse_iterator<raw_const_pointer> raw_rbegin() const noexcept;
		std::reverse_iterator<raw_const_pointer> raw_crbegin() const noexcept;
		std::reverse_iterator<raw_const_pointer> raw_rend() const noexcept;
		std::reverse_iterator<raw_const_pointer> raw_crend() const noexcept;

		constexpr const_cursor cursor_begin() const;
		constexpr const_cursor cursor_end() const;
		constexpr const_iterator iter() const;
		constexpr const_reverse_iterator iter_inv() const;
		constexpr const_range range() const;
		constexpr const_reverse_range range_inv() const;

		//==================> size <==================

		constexpr bool empty() const noexcept;
		constexpr size_type length() const noexcept;
		constexpr size_type text_length() const noexcept;
		constexpr size_type size() const noexcept;
		constexpr size_type max_size() const noexcept;
		template<is_char_v Char> constexpr size_type to_size() const noexcept;

		//==================> data access <==================

		constexpr const_reference at(size_type pos) const;
		constexpr const_reference front() const;
		constexpr const_reference back() const;
		constexpr const_pointer data() const noexcept;
		constexpr std::u8string_view view() const noexcept;

		raw_const_reference raw_at(size_type pos) const;
		raw_const_reference raw_front() const;
		raw_const_reference raw_back() const;
		raw_const_pointer raw_data() const noexcept;
		std::string_view raw_view() const noexcept;

		constexpr const_reference operator[](size_type pos) const;
		constexpr UTF8Seq at_text(size_type pos) const;
		constexpr UTF8Seq last_text(size_type index) const;
		constexpr bool is_valid_index(size_type index) const noexcept;
		constexpr size_type buffer_index_to_text(size_type index) const noexcept;
		constexpr size_type text_index_to_buffer(size_type index) const noexcept;

		//==================> remove prefix & prefix <==================

		constexpr void remove_prefix(size_type n);
		constexpr void remove_prefix(u8string_view prefix);
		constexpr void remove_prefix(UTF8Seq prefix);

		constexpr u8string_view RemovePrefix(size_type n) const;
		constexpr u8string_view RemovePrefix(u8string_view prefix) const;
		constexpr u8string_view RemovePrefix(UTF8Seq prefix) const;

		constexpr void remove_suffix(size_type n);
		constexpr void remove_suffix(u8string_view suffix);
		constexpr void remove_suffix(UTF8Seq suffix);

		constexpr u8string_view RemoveSuffix(size_type n) const;
		constexpr u8string_view RemoveSuffix(u8string_view suffix) const;
		constexpr u8string_view RemoveSuffix(UTF8Seq suffix) const;

		//==================> subview <==================

		constexpr u8string_view first_view(size_type count) const;
		constexpr u8string_view last_view(size_type count) const;
		constexpr u8string_view subview(size_type pos = 0, size_type count = npos) const;

		//==================> starts & ends with <==================

		constexpr bool starts_with(u8string_view sv) const noexcept;
		constexpr bool starts_with(value_type ch) const noexcept;
		constexpr bool starts_with(UTF8Seq prefix) const;

		constexpr bool ends_with(u8string_view sv) const noexcept;
		constexpr bool ends_with(value_type ch) const noexcept;
		constexpr bool ends_with(UTF8Seq suffix) const;

		//==================> contains & count <==================

		constexpr bool contains(u8string_view sv) const noexcept;
		constexpr bool contains(value_type ch) const noexcept;
		constexpr bool contains(UTF8Seq seq) const;

		constexpr size_type count(u8string_view pattern) const;
		constexpr size_type count(UTF8Seq seq) const;

		//==================> find <==================

		constexpr const_data_reference find(u8string_view v, size_type pos = 0) const noexcept;
		constexpr const_data_reference find(value_type ch, size_type pos = 0) const noexcept;
		constexpr const_data_reference find(UTF8Seq pattern, size_type pos = 0) const;
		constexpr const_data_reference find(const_pointer s, size_type pos, size_type count) const;

		constexpr const_data_reference find_first_of(u8string_view v, size_type pos = 0) const noexcept;
		constexpr const_data_reference find_first_of(value_type ch, size_type pos = 0) const noexcept;
		constexpr const_data_reference find_first_of(UTF8Seq pattern, size_type pos = 0) const;
		constexpr const_data_reference find_first_of(const_pointer s, size_type pos, size_type count) const;

		constexpr const_data_reference find_first_not_of(u8string_view v, size_type pos = 0) const noexcept;
		constexpr const_data_reference find_first_not_of(value_type ch, size_type pos = 0) const noexcept;
		constexpr const_data_reference find_first_not_of(UTF8Seq pattern, size_type pos = 0) const;
		constexpr const_data_reference find_first_not_of(const_pointer s, size_type pos, size_type count) const;

		constexpr const_data_reference rfind(u8string_view v, size_type pos = npos) const noexcept;
		constexpr const_data_reference rfind(value_type ch, size_type pos = npos) const noexcept;
		constexpr const_data_reference rfind(UTF8Seq pattern, size_type pos = npos) const;
		constexpr const_data_reference rfind(const_pointer s, size_type pos, size_type count) const;

		constexpr const_data_reference find_last_of(u8string_view v, size_type pos = npos) const noexcept;
		constexpr const_data_reference find_last_of(value_type ch, size_type pos = npos) const noexcept;
		constexpr const_data_reference find_last_of(UTF8Seq pattern, size_type pos = npos) const;
		constexpr const_data_reference find_last_of(const_pointer s, size_type pos, size_type count) const;

		constexpr const_data_reference find_last_not_of(u8string_view v, size_type pos = npos) const noexcept;
		constexpr const_data_reference find_last_not_of(value_type ch, size_type pos = npos) const noexcept;
		constexpr const_data_reference find_last_not_of(UTF8Seq pattern, size_type pos = npos) const;
		constexpr const_data_reference find_last_not_of(const_pointer s, size_type pos, size_type count) const;

		//==================> partition <==================

		constexpr std::array<u8string_view, 3> partition(u8string_view delimiter) const;
		constexpr std::array<u8string_view, 3> partition(UTF8Seq delimiter) const;

		//==================> trim <==================

		constexpr u8string_view trim(u8string_view characters = u8" \t") const;
		constexpr u8string_view trim_start(u8string_view characters = u8" \t") const;
		constexpr u8string_view trim_end(u8string_view characters = u8" \t") const;
		constexpr u8string_view trim(UTF8Seq seq) const;
		constexpr u8string_view trim_start(UTF8Seq seq) const;
		constexpr u8string_view trim_end(UTF8Seq seq) const;
		constexpr u8string_view trim_invalid() const;
		constexpr u8string_view trim_invalid_start() const;
		constexpr u8string_view trim_invalid_end() const;

		//==================> split <==================

		template<internal::CanAdd<u8string_view> Buffer>
		constexpr size_type split(Buffer& out, u8string_view delimiter, bool cull_empty = false, size_type limit = npos) const;

		template<internal::CanAdd<u8string_view> Buffer>
		constexpr size_type split(Buffer& out, UTF8Seq delimiter, bool cull_empty = false, size_type limit = npos) const;

		template<std::invocable<u8string_view> F>
		constexpr size_type split_each(F&& func, u8string_view delimiter, bool cull_empty = false, size_type limit = npos) const;

		template<std::invocable<u8string_view> F>
		constexpr size_type split_each(F&& func, UTF8Seq delimiter, bool cull_empty = false, size_type limit = npos) const;

		//==================> misc <==================

		constexpr void swap(u8string_view& v) noexcept;
		constexpr size_type copy(pointer dest, size_type count, size_type pos = 0) const;

	private:
		std::u8string_view data_;
	};

	template<>
	struct formatter<u8string_view> : formatter<std::u8string_view> {
		using base = formatter<std::u8string_view>;

		context::iterator format(u8string_view value, context& ctx) const {
			return base::format(std::u8string_view{value.data(), value.size()}, ctx);
		}
	};
}

#include "implement/string_view.inl"
