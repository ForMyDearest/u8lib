#pragma once

#include "config.hpp"
#include "string_view.hpp"

namespace u8lib
{
	//! @note Strictly prohibit empty assignment
	class u8string {
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

		using traits_type = std::char_traits<char8_t>;
		using allocator_type = std::allocator<char8_t>;

		using data_reference = VectorDataRef<value_type, false>;
		using const_data_reference = VectorDataRef<value_type, true>;
		using cursor = UTF8Cursor<false>;
		using const_cursor = UTF8Cursor<true>;
		using iterator = UTF8Iter<false>;
		using const_iterator = UTF8Iter<true>;
		using reverse_iterator = UTF8IterInv<false>;
		using const_reverse_iterator = UTF8IterInv<true>;
		using range_t = UTF8Range<false>;
		using const_range = UTF8Range<true>;
		using reverse_range = UTF8RangeInv<false>;
		using const_reverse_range = UTF8RangeInv<true>;

		static constexpr size_type SSOSize = 31;
		static constexpr size_type SSOBufferSize = SSOSize + 1;
		static constexpr size_type SSOCapacity = SSOSize - 1;
		static constexpr size_type npos = u8string_view::npos;

		static_assert(SSOBufferSize % 4 == 0, "SSOSize must be 4n - 1");
		static_assert(SSOBufferSize > sizeof(size_type) * 2 + sizeof(pointer), "SSOSize must be larger than heap data size");
		static_assert(SSOBufferSize < 128, "SSOBufferSize must be less than 127"); // sso_size_ max

		//==================> join <==================

		template<typename... Args> static u8string concat(Args&&... string_or_view);
		template<typename Container> static u8string join(const Container& container, u8string_view separator, bool skip_empty = true, u8string_view trim_chs = {});

		//==================> ctor & dtor <==================

		U8LIB_API u8string();
		U8LIB_API u8string(size_type count, value_type ch);
		u8string(const u8string& other);
		U8LIB_API u8string(u8string&& rhs) noexcept;

		u8string(const char* str);
		u8string(const char8_t* str);
		u8string(const char16_t* str);
		u8string(const char32_t* str);
		u8string(const wchar_t* str);

		u8string(const char* str, size_type count);
		u8string(const char8_t* str, size_type count);
		U8LIB_API u8string(const char16_t* str, size_type count);
		U8LIB_API u8string(const char32_t* str, size_type count);
		u8string(const wchar_t* str, size_type count);

		U8LIB_API u8string(u8string_view view);
		u8string(u8string_view view, size_type pos);
		u8string(u8string_view view, size_type pos, size_type count);

		U8LIB_API ~u8string() noexcept;

		//==================> assign <==================

		U8LIB_API u8string& assign(u8string&& rhs) noexcept;
		U8LIB_API u8string& assign(u8string_view view);
		u8string& assign(u8string_view view, size_type pos, size_type count = npos);

		u8string& assign(const char* str);
		u8string& assign(const char8_t* str);
		u8string& assign(const char16_t* str);
		u8string& assign(const char32_t* str);
		u8string& assign(const wchar_t* str);

		u8string& assign(const char* str, size_type count);
		u8string& assign(const char8_t* str, size_type count);
		U8LIB_API u8string& assign(const char16_t* str, size_type count);
		U8LIB_API u8string& assign(const char32_t* str, size_type count);
		u8string& assign(const wchar_t* str, size_type count);

		u8string& operator=(const u8string& rhs);
		u8string& operator=(u8string&& rhs) noexcept;
		u8string& operator=(u8string_view view);
		u8string& operator=(const char* str);
		u8string& operator=(const char8_t* str);
		u8string& operator=(const char16_t* str);
		u8string& operator=(const char32_t* str);
		u8string& operator=(const wchar_t* str);

		//==================> compare <==================

		bool operator==(const char* str) const noexcept;
		bool operator==(const char8_t* str) const noexcept;
		bool operator==(u8string_view str) const noexcept;
		bool operator==(const u8string& str) const noexcept;
		std::strong_ordering operator<=>(const char* str) const noexcept;
		std::strong_ordering operator<=>(const char8_t* str) const noexcept;
		std::strong_ordering operator<=>(u8string_view str) const noexcept;
		std::strong_ordering operator<=>(const u8string& str) const noexcept;

		//==================> iterator <==================

		pointer begin() noexcept;
		const_pointer begin() const noexcept;
		const_pointer cbegin() const noexcept;

		pointer end() noexcept;
		const_pointer end() const noexcept;
		const_pointer cend() const noexcept;

		std::reverse_iterator<pointer> rbegin() noexcept;
		std::reverse_iterator<const_pointer> rbegin() const noexcept;
		std::reverse_iterator<const_pointer> crbegin() const noexcept;

		std::reverse_iterator<pointer> rend() noexcept;
		std::reverse_iterator<const_pointer> rend() const noexcept;
		std::reverse_iterator<const_pointer> crend() const noexcept;

		cursor cursor_begin();
		cursor cursor_end();
		iterator iter();
		reverse_iterator iter_inv();
		range_t range();
		reverse_range range_inv();

		const_cursor cursor_begin() const;
		const_cursor cursor_end() const;
		const_iterator iter() const;
		const_reverse_iterator iter_inv() const;
		const_range range() const;
		const_reverse_range range_inv() const;

		//==================> size <==================

		bool empty() const noexcept;
		size_type length() const noexcept;
		size_type text_length() const noexcept;
		size_type capacity() const noexcept;
		size_type slack() const noexcept;
		size_type size() const noexcept;
		size_type max_size() const noexcept;
		template<is_char_v Char> size_type to_size() const noexcept;

		//==================> data access <==================

		reference at(size_type pos);
		reference operator[](size_type pos);
		reference front();
		reference back();
		pointer data() noexcept;

		const_reference at(size_type pos) const;
		const_reference operator[](size_type pos) const;
		const_reference front() const;
		const_reference back() const;
		const_pointer data() const noexcept;

		raw_reference raw_at(size_type pos);
		raw_reference raw_front();
		raw_reference raw_back();
		raw_pointer raw_data() noexcept;

		raw_const_reference raw_at(size_type pos) const;
		raw_const_reference raw_front() const;
		raw_const_reference raw_back() const;
		raw_const_pointer raw_data() const noexcept;

		const_pointer c_str() const noexcept;
		UTF8Seq at_text(size_type index) const;
		UTF8Seq last_text(size_type index) const;
		bool is_sso() const noexcept;
		bool is_heap() const noexcept;
		bool is_valid_index(size_type index) const noexcept;
		size_type buffer_index_to_text(size_type index) const noexcept;
		size_type text_index_to_buffer(size_type index) const noexcept;

		//==================> sub string & view <==================

		operator u8string_view() const noexcept;
		u8string_view first_view(size_type count) const;
		u8string_view last_view(size_type count) const;
		u8string_view subview(size_type start, size_type count = npos) const noexcept;
		u8string first_str(size_type count) const;
		u8string last_str(size_type count) const;
		u8string substr(size_type pos = 0, size_type count = npos) const;

		//==================> add <==================

		U8LIB_API u8string& insert(size_type index, size_type count, value_type ch);
		u8string& insert(size_type index, UTF8Seq seq);
		U8LIB_API u8string& insert(size_type index, u8string_view view);
		U8LIB_API u8string& insert(size_type index, const char16_t* str);
		U8LIB_API u8string& insert(size_type index, const char32_t* str);
		u8string& insert(size_type index, const wchar_t* str);

		U8LIB_API u8string& append(size_type count, value_type ch);
		u8string& append(UTF8Seq seq);
		U8LIB_API u8string& append(u8string_view view);
		U8LIB_API u8string& append(const char16_t* str);
		U8LIB_API u8string& append(const char32_t* str);
		u8string& append(const wchar_t* str);

		u8string& push_back(value_type ch);

		void operator+=(value_type ch);
		void operator+=(UTF8Seq seq);
		void operator+=(u8string_view view);
		void operator+=(const char16_t* str);
		void operator+=(const char32_t* str);
		void operator+=(const wchar_t* str);

		//==================> remove <==================

		U8LIB_API u8string& clear() noexcept;
		U8LIB_API u8string& pop_back(size_type count = 1);
		U8LIB_API u8string& erase(size_type index = 0, size_type count = npos);

		//==================> replace <==================

		U8LIB_API u8string& replace(size_type pos, size_type count, const_pointer cstr, size_type count2);
		u8string& replace(size_type pos, size_type count, u8string_view view);

		u8string Replace(size_type pos, size_type count, const_pointer cstr, size_type count2) const;
		u8string Replace(size_type pos, size_type count, u8string_view view) const;

		//==================> starts & ends with <==================

		bool starts_with(u8string_view sv) const noexcept;
		bool starts_with(value_type ch) const noexcept;
		bool starts_with(UTF8Seq seq) const;

		bool ends_with(u8string_view sv) const noexcept;
		bool ends_with(value_type ch) const noexcept;
		bool ends_with(UTF8Seq seq) const;

		//==================> contains & count <==================

		bool contains(u8string_view sv) const noexcept;
		bool contains(value_type ch) const noexcept;
		bool contains(UTF8Seq seq) const;

		size_type count(u8string_view pattern) const;
		size_type count(UTF8Seq seq) const;

		//==================> find <==================

		data_reference find(u8string_view v, size_type pos = 0) noexcept;
		data_reference find(value_type ch, size_type pos = 0) noexcept;
		data_reference find(UTF8Seq seq, size_type pos = 0);
		data_reference find(const_pointer s, size_type pos, size_type count);

		data_reference find_first_of(u8string_view v, size_type pos = 0) noexcept;
		data_reference find_first_of(value_type ch, size_type pos = 0) noexcept;
		data_reference find_first_of(UTF8Seq seq, size_type pos = 0);
		data_reference find_first_of(const_pointer s, size_type pos, size_type count);

		data_reference find_first_not_of(u8string_view v, size_type pos = 0) noexcept;
		data_reference find_first_not_of(value_type ch, size_type pos = 0) noexcept;
		data_reference find_first_not_of(UTF8Seq seq, size_type pos = 0);
		data_reference find_first_not_of(const_pointer s, size_type pos, size_type count);

		data_reference rfind(u8string_view v, size_type pos = npos) noexcept;
		data_reference rfind(value_type ch, size_type pos = npos) noexcept;
		data_reference rfind(UTF8Seq seq, size_type pos = npos);
		data_reference rfind(const_pointer s, size_type pos, size_type count);

		data_reference find_last_of(u8string_view v, size_type pos = npos) noexcept;
		data_reference find_last_of(value_type ch, size_type pos = npos) noexcept;
		data_reference find_last_of(UTF8Seq seq, size_type pos = npos);
		data_reference find_last_of(const_pointer s, size_type pos, size_type count);

		data_reference find_last_not_of(u8string_view v, size_type pos = npos) noexcept;
		data_reference find_last_not_of(value_type ch, size_type pos = npos) noexcept;
		data_reference find_last_not_of(UTF8Seq seq, size_type pos = npos);
		data_reference find_last_not_of(const_pointer s, size_type pos, size_type count);

		const_data_reference find(u8string_view v, size_type pos = 0) const noexcept;
		const_data_reference find(value_type ch, size_type pos = 0) const noexcept;
		const_data_reference find(UTF8Seq seq, size_type pos = 0) const;
		const_data_reference find(const_pointer s, size_type pos, size_type count) const;

		const_data_reference find_first_of(u8string_view v, size_type pos = 0) const noexcept;
		const_data_reference find_first_of(value_type ch, size_type pos = 0) const noexcept;
		const_data_reference find_first_of(UTF8Seq seq, size_type pos = 0) const;
		const_data_reference find_first_of(const_pointer s, size_type pos, size_type count) const;

		const_data_reference find_first_not_of(u8string_view v, size_type pos = 0) const noexcept;
		const_data_reference find_first_not_of(value_type ch, size_type pos = 0) const noexcept;
		const_data_reference find_first_not_of(UTF8Seq seq, size_type pos = 0) const;
		const_data_reference find_first_not_of(const_pointer, size_type pos, size_type count) const;

		const_data_reference rfind(u8string_view v, size_type pos = npos) const noexcept;
		const_data_reference rfind(value_type ch, size_type pos = npos) const noexcept;
		const_data_reference rfind(UTF8Seq seq, size_type pos = npos) const;
		const_data_reference rfind(const_pointer, size_type pos, size_type count) const;

		const_data_reference find_last_of(u8string_view v, size_type pos = npos) const noexcept;
		const_data_reference find_last_of(value_type ch, size_type pos = npos) const noexcept;
		const_data_reference find_last_of(UTF8Seq seq, size_type pos = npos) const;
		const_data_reference find_last_of(const_pointer s, size_type pos, size_type count) const;

		const_data_reference find_last_not_of(u8string_view v, size_type pos = npos) const noexcept;
		const_data_reference find_last_not_of(value_type ch, size_type pos = npos) const noexcept;
		const_data_reference find_last_not_of(UTF8Seq seq, size_type pos = npos) const;
		const_data_reference find_last_not_of(const_pointer s, size_type pos, size_type count) const;

		//==================> remove prefix & prefix <==================

		u8string& remove_prefix(size_type n);
		u8string& remove_prefix(u8string_view prefix);
		u8string& remove_prefix(UTF8Seq prefix);

		u8string& remove_suffix(size_type n);
		u8string& remove_suffix(u8string_view suffix);
		u8string& remove_suffix(UTF8Seq suffix);

		u8string RemovePrefix(size_type n) const;
		u8string RemovePrefix(u8string_view prefix) const;
		u8string RemovePrefix(UTF8Seq prefix) const;

		u8string RemoveSuffix(size_type n) const;
		u8string RemoveSuffix(u8string_view suffix) const;
		u8string RemoveSuffix(UTF8Seq suffix) const;

		//==================> partition <==================

		std::array<u8string_view, 3> partition(u8string_view delimiter) const;
		std::array<u8string_view, 3> partition(UTF8Seq delimiter) const;

		//==================> trim <==================

		u8string& trim(u8string_view characters = u8" \t");
		u8string& trim_start(u8string_view characters = u8" \t");
		u8string& trim_end(u8string_view characters = u8" \t");
		u8string& trim(UTF8Seq seq);
		u8string& trim_start(UTF8Seq seq);
		u8string& trim_end(UTF8Seq seq);
		u8string& trim_invalid();
		u8string& trim_invalid_start();
		u8string& trim_invalid_end();

		u8string Trim(u8string_view characters = u8" \t") const;
		u8string TrimStart(u8string_view characters = u8" \t") const;
		u8string TrimEnd(u8string_view characters = u8" \t") const;
		u8string Trim(UTF8Seq seq) const;
		u8string TrimStart(UTF8Seq seq) const;
		u8string TrimEnd(UTF8Seq seq) const;
		u8string TrimInvalid() const;
		u8string TrimInvalidStart() const;
		u8string TrimInvalidEnd() const;

		//==================> split <==================

		template<internal::CanAdd<u8string_view> Buffer>
		size_type split(Buffer& out, u8string_view delimiter, bool cull_empty = false, size_type limit = npos) const;

		template<std::invocable<u8string_view> F>
		size_type split_each(F&& func, u8string_view delimiter, bool cull_empty = false, size_type limit = npos) const;

		template<internal::CanAdd<u8string_view> Buffer>
		size_type split(Buffer& out, UTF8Seq delimiter, bool cull_empty = false, size_type limit = npos) const;

		template<std::invocable<u8string_view> F>
		size_type split_each(F&& func, UTF8Seq delimiter, bool cull_empty = false, size_type limit = npos) const;

		//==================> misc <==================

		U8LIB_API void reserve(size_type new_cap);
		void release(size_type reserve_capacity = 0);
		U8LIB_API void resize(size_type count);
		U8LIB_API void resize(size_type count, value_type ch);
		void swap(u8string& other) noexcept;
		u8string& reverse(size_type start = 0, size_type count = npos);
		size_type copy(pointer dest, size_type count, size_type pos = 0) const;

	private:
		friend struct StringHelper;

		union {
			struct {
				pointer data_;
				size_type size_;
				size_type capacity_;
			};

			struct {
				value_type sso_data_[SSOSize];
				uint8_t sso_flag_: 1;
				uint8_t sso_size_: 7;
			};

			uint8_t buffer_[SSOBufferSize];
		};
	};

	template<>
	struct formatter<std::u8string> : formatter<std::u8string_view> {};

	template<>
	struct formatter<std::string> : formatter<std::string_view> {};

	template<>
	struct formatter<u8string> : formatter<std::u8string_view> {
		using base = formatter<std::u8string_view>;

		context::iterator format(const u8string& value, context& ctx) const {
			return base::format(std::u8string_view{value.data(), value.size()}, ctx);
		}
	};

	namespace internal
	{
		U8LIB_API u8string vformat(std::u8string_view fmt, format_args args, const void* loc = nullptr);
	}

	inline u8string vformat(std::u8string_view fmt, format_args args) {
		return internal::vformat(fmt, args);
	}

	template<typename... T>
	u8string format(format_string<T...> fmt, T&&... args) {
		constexpr auto DESC = internal::make_descriptor<T...>();
		return u8lib::vformat(fmt.get(), format_args(u8lib::make_format_store(args...), DESC));
	}
}

#include "implement/string.inl"
