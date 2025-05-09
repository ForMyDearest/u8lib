#pragma once

// TODO : 1.支持全部基础字符类型
// TODO : 2.支持 std 容器交互
// TODO : 3.宽松的约束，导致编译代价增高

#include "auxiliary/utility/make_aligned.hpp"
#include "auxiliary/unicode/u8string_view.hpp"

namespace auxiliary::unicode
{
	struct DefaultPolicy {
		static constexpr size_t get_grow(size_t expect_size) {
			constexpr size_t at_least_grow = 16;
			const auto ret = make_aligned(expect_size / 2 + expect_size, sizeof(size_t)) + at_least_grow;

			// overflow
			AUXILIARY_ASSERT(expect_size < ret);

			return ret;
		}

		static constexpr size_t get_reserve(size_t expect_size) {
			return make_aligned(expect_size, sizeof(size_t));
		}
	};

	//! @note Strictly prohibit empty assignment
	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	class U8String : protected Allocator {
	public:
		//==================> aligns <==================

		using traits_type = Traits;
		using allocator_type = Allocator;
		using policy_type = ContainerPolicy;
		using view_type = U8StringView<traits_type>;

		using value_type = char8_t;
		using pointer = value_type*;
		using const_pointer = const value_type*;
		using reference = value_type&;
		using const_reference = const value_type&;
		using size_type = size_t;
		using difference_type = ptrdiff_t;

		using cursor = UTF8Cursor<false>;
		using const_cursor = UTF8Cursor<true>;
		using iterator = UTF8Iter<false>;
		using const_iterator = UTF8Iter<true>;
		using reverse_iterator = UTF8IterInv<true>;
		using const_reverse_iterator = UTF8IterInv<true>;
		using range = UTF8Range<false>;
		using const_range = UTF8Range<true>;
		using reverse_range = UTF8RangeInv<true>;
		using const_reverse_range = UTF8RangeInv<true>;

		static constexpr size_type SSOBufferSize = SSOSize + 1;
		static constexpr size_type SSOCapacity = SSOSize - 1;
		static constexpr size_type npos = view_type::npos;

		static_assert(SSOBufferSize % 4 == 0, "SSOSize must be 4n - 1");
		static_assert(SSOBufferSize > sizeof(size_type) * 2 + sizeof(pointer), "SSOSize must be larger than heap data size");
		static_assert(SSOBufferSize < 128, "SSOBufferSize must be less than 127"); // sso_size_ max

		//==================> join <==================

		template<typename... Args> static constexpr U8String concat(Args&&... string_or_view);
		template<typename Container> static constexpr U8String join(const Container& container, view_type separator, bool skip_empty = true, view_type trim_chs = {});

		//==================> ctor & dtor <==================

		constexpr U8String();
		constexpr U8String(size_type count, value_type ch);
		template<is_char_v Char> constexpr U8String(const Char* str);
		template<is_char_v Char> constexpr U8String(const Char* str, size_type len);

		template<StrViewLike<false> View> constexpr U8String(View view);
		template<StrViewLike<false> View> constexpr U8String(View view, size_type pos);
		template<StrViewLike<false> View> constexpr U8String(View view, size_type pos, size_type count);

		template<StrViewLike<true> Str> constexpr U8String(const Str& str);
		template<StrViewLike<true> Str> constexpr U8String(const Str& str, size_type pos);
		template<StrViewLike<true> Str> constexpr U8String(const Str& str, size_type pos, size_type count);

		constexpr U8String(const U8String& rhs);
		constexpr U8String(U8String&& rhs) noexcept;

		template<typename A> explicit constexpr U8String(A&& a) requires(std::is_constructible_v<Allocator, A>);
		template<typename A> constexpr U8String(size_type count, value_type ch, A&& a) requires(std::is_constructible_v<Allocator, A>);
		template<typename A, is_char_v Char> constexpr U8String(const Char* str, A&& a) requires(std::is_constructible_v<Allocator, A>);
		template<typename A, is_char_v Char> constexpr U8String(const Char* str, size_type len, A&& a) requires(std::is_constructible_v<Allocator, A>);

		template<typename A, StrViewLike<false> View> constexpr U8String(View view, A&& a) requires(std::is_constructible_v<Allocator, A>);
		template<typename A, StrViewLike<false> View> constexpr U8String(View view, size_type pos, A&& a) requires(std::is_constructible_v<Allocator, A>);
		template<typename A, StrViewLike<false> View> constexpr U8String(View view, size_type pos, size_type count, A&& a) requires(std::is_constructible_v<Allocator, A>);

		template<typename A, StrViewLike<true> Str> constexpr U8String(const Str& str, A&& a) requires(std::is_constructible_v<Allocator, A>);
		template<typename A, StrViewLike<true> Str> constexpr U8String(const Str& str, size_type pos, A&& a) requires(std::is_constructible_v<Allocator, A>);
		template<typename A, StrViewLike<true> Str> constexpr U8String(const Str& str, size_type pos, size_type count, A&& a) requires(std::is_constructible_v<Allocator, A>);

		template<typename A> constexpr U8String(U8String&& rhs, A&& a) requires(std::is_constructible_v<Allocator, A>);

		U8String(std::nullptr_t) = delete;
		U8String(std::nullptr_t, size_type) = delete;
		template<typename A> U8String(std::nullptr_t, A&&) = delete;
		template<typename A> U8String(std::nullptr_t, size_type, A&&) = delete;

		constexpr ~U8String() noexcept;

		//==================> assign <==================

		constexpr U8String& operator=(const U8String& rhs);
		constexpr U8String& operator=(U8String&& rhs) noexcept;
		template<is_char_v Char> constexpr U8String& operator=(const Char* str);
		template<StrViewLike<false> View> constexpr U8String& operator=(View view);
		template<StrViewLike<true> Str> constexpr U8String& operator=(const Str& str);
		constexpr U8String& operator=(std::nullptr_t) = delete;

		constexpr U8String& assign(U8String&& rhs) noexcept;
		template<is_char_v Char> constexpr U8String& assign(const Char* str);
		template<is_char_v Char> constexpr U8String& assign(const Char* str, size_type len);
		template<StrViewLike<false> View> constexpr U8String& assign(View view);
		template<StrViewLike<true> Str> constexpr U8String& assign(const Str& str);
		constexpr U8String& assign(std::nullptr_t) = delete;
		constexpr U8String& assign(std::nullptr_t, size_type) = delete;

		//==================> compare <==================

		constexpr bool operator==(const_pointer str) const noexcept;
		constexpr bool operator==(const U8String& str) const noexcept;
		constexpr bool operator==(view_type str) const noexcept;

		constexpr std::strong_ordering operator<=>(const U8String& str) const noexcept;
		constexpr std::strong_ordering operator<=>(view_type str) const noexcept;
		constexpr std::strong_ordering operator<=>(const_pointer str) const noexcept;

		constexpr int compare(view_type v) const noexcept;

		//==================> iterator <==================

		constexpr pointer begin() noexcept;
		constexpr const_pointer begin() const noexcept;
		constexpr const_pointer cbegin() const noexcept;

		constexpr pointer end() noexcept;
		constexpr const_pointer end() const noexcept;
		constexpr const_pointer cend() const noexcept;

		constexpr std::reverse_iterator<pointer> rbegin() noexcept;
		constexpr std::reverse_iterator<const_pointer> rbegin() const noexcept;
		constexpr std::reverse_iterator<const_pointer> crbegin() const noexcept;

		constexpr std::reverse_iterator<pointer> rend() noexcept;
		constexpr std::reverse_iterator<const_pointer> rend() const noexcept;
		constexpr std::reverse_iterator<const_pointer> crend() const noexcept;

		//==================> size <==================

		constexpr bool empty() const noexcept;
		constexpr size_type length() const noexcept;
		constexpr size_type length_text() const noexcept;
		constexpr size_type capacity() const noexcept;
		constexpr size_type slack() const noexcept;
		constexpr size_type size() const noexcept;
		constexpr size_type max_size() const noexcept;
		template<is_char_v Char> constexpr size_type to_size() const noexcept;

		//==================> data access <==================

		constexpr reference at(size_type pos);
		constexpr reference last_buffer(size_type pos);
		constexpr reference operator[](size_type pos);
		constexpr reference front();
		constexpr reference back();
		constexpr pointer data() noexcept;
		constexpr UTF8Seq at_text(size_type index) const;
		constexpr UTF8Seq last_text(size_type index) const;

		constexpr allocator_type get_allocator() const noexcept;
		constexpr const_reference at(size_type pos) const;
		constexpr const_reference last_buffer(size_type pos) const;
		constexpr const_reference operator[](size_type pos) const;
		constexpr const_reference front() const;
		constexpr const_reference back() const;
		constexpr const_pointer data() const noexcept;
		constexpr const char* raw_data() const noexcept;
		constexpr const_pointer c_str() const noexcept;

		constexpr bool is_sso() const noexcept;
		constexpr bool is_heap() const noexcept;
		constexpr bool is_valid_index(size_type index) const noexcept;
		constexpr size_type buffer_index_to_text(size_type index) const noexcept;
		constexpr size_type text_index_to_buffer(size_type index) const noexcept;

		//==================> sub string & view <==================

		constexpr operator view_type() const noexcept;
		constexpr view_type view() const noexcept;
		constexpr view_type first_view(size_type count) const;
		constexpr view_type last_view(size_type count) const;
		constexpr view_type subview(size_type start, size_type count = npos) const noexcept;
		constexpr U8String first(size_type count) const;
		constexpr U8String last(size_type count) const;
		constexpr U8String substr(size_type pos = 0, size_type count = npos) const;

		//==================> modify <==================

		constexpr void reserve(size_type new_cap);
		constexpr void shrink_to_fit();
		constexpr void release(size_type reserve_capacity = 0);
		constexpr void resize(size_type count);
		constexpr void resize(size_type count, value_type ch);
		constexpr void swap(U8String& other) noexcept;
		constexpr U8String& reverse(size_type start = 0, size_type count = npos);

		constexpr size_type copy(pointer dest, size_type count, size_type pos = 0) const;

		//==================> add <==================

		constexpr U8String& insert(size_type index, size_type count, value_type ch);
		constexpr U8String& insert(size_type index, UTF8Seq seq);
		template<is_char_v Char> constexpr U8String& insert(size_type index, const Char* s);
		template<is_char_v Char> constexpr U8String& insert(size_type index, const Char* s, size_type len);
		template<StrViewLike<false> View> constexpr U8String& insert(size_type index, View view);
		template<StrViewLike<true> Str> constexpr U8String& insert(size_type index, const Str& str);

		constexpr U8String& push_back(value_type ch);
		constexpr U8String& append(UTF8Seq seq);
		constexpr U8String& append(size_type count, value_type ch);
		template<is_char_v Char> constexpr U8String& append(const Char* str);
		template<is_char_v Char> constexpr U8String& append(const Char* str, size_type len);
		template<StrViewLike<false> View> constexpr U8String& append(View view);
		template<StrViewLike<true> Str> constexpr U8String& append(const Str& str);

		constexpr void operator+=(value_type ch);
		constexpr void operator+=(UTF8Seq seq);
		template<is_char_v Char> constexpr void operator+=(const Char* s);
		template<StrViewLike<false> View> constexpr void operator+=(View view);
		template<StrViewLike<true> Str> constexpr void operator+=(const Str& str);

		//==================> remove <==================

		constexpr U8String& pop_back(size_type count = 1);
		constexpr U8String& clear() noexcept;
		constexpr U8String& erase(size_type index = 0, size_type count = npos);

		//==================> replace <==================

		constexpr U8String& replace(size_type pos, size_type count, const_pointer cstr, size_type count2);
		constexpr U8String& replace(size_type pos, size_type count, view_type view);

		constexpr U8String Replace(size_type pos, size_type count, const_pointer cstr, size_type count2) const;
		constexpr U8String Replace(size_type pos, size_type count, view_type view) const;

		//==================> starts & ends with <==================

		constexpr bool starts_with(view_type sv) const noexcept;
		constexpr bool starts_with(value_type ch) const noexcept;
		constexpr bool starts_with(UTF8Seq seq) const noexcept;

		constexpr bool ends_with(view_type sv) const noexcept;
		constexpr bool ends_with(value_type ch) const noexcept;
		constexpr bool ends_with(UTF8Seq seq) const noexcept;

		//==================> count & find <==================

		constexpr bool contains(view_type sv) const noexcept;
		constexpr bool contains(value_type ch) const noexcept;
		constexpr bool contains(UTF8Seq seq) const noexcept;

		constexpr size_type count(view_type pattern) const;
		constexpr size_type count(UTF8Seq seq) const;

		constexpr size_type find(view_type v, size_type pos = 0) const noexcept;
		constexpr size_type find(UTF8Seq seq, size_type pos = 0) const noexcept;
		constexpr size_type find(value_type ch, size_type pos = 0) const noexcept;
		constexpr size_type find(const_pointer s, size_type pos, size_type count) const;

		constexpr size_type find_last(view_type pattern) const;
		constexpr size_type find_last(UTF8Seq seq) const;

		constexpr size_type rfind(view_type v, size_type pos = npos) const noexcept;
		constexpr size_type rfind(UTF8Seq seq, size_type pos = npos) const noexcept;
		constexpr size_type rfind(value_type ch, size_type pos = npos) const noexcept;
		constexpr size_type rfind(const_pointer s, size_type pos, size_type count) const;

		constexpr size_type find_first_of(view_type v, size_type pos = 0) const noexcept;
		constexpr size_type find_first_of(UTF8Seq seq, size_type pos = 0) const noexcept;
		constexpr size_type find_first_of(value_type ch, size_type pos = 0) const noexcept;
		constexpr size_type find_first_of(const_pointer s, size_type pos, size_type count) const;

		constexpr size_type find_last_of(view_type v, size_type pos = npos) const noexcept;
		constexpr size_type find_last_of(UTF8Seq seq, size_type pos = npos) const noexcept;
		constexpr size_type find_last_of(value_type ch, size_type pos = npos) const noexcept;
		constexpr size_type find_last_of(const_pointer s, size_type pos, size_type count) const;

		constexpr size_type find_first_not_of(view_type v, size_type pos = 0) const noexcept;
		constexpr size_type find_first_not_of(UTF8Seq seq, size_type pos = 0) const noexcept;
		constexpr size_type find_first_not_of(value_type ch, size_type pos = 0) const noexcept;
		constexpr size_type find_first_not_of(const_pointer s, size_type pos, size_type count) const;

		constexpr size_type find_last_not_of(view_type v, size_type pos = npos) const noexcept;
		constexpr size_type find_last_not_of(UTF8Seq seq, size_type pos = npos) const noexcept;
		constexpr size_type find_last_not_of(value_type ch, size_type pos = npos) const noexcept;
		constexpr size_type find_last_not_of(const_pointer s, size_type pos, size_type count) const;

		//==================> remove prefix & suffix <==================

		constexpr U8String& remove_prefix(view_type prefix);
		constexpr U8String& remove_prefix(UTF8Seq seq);
		constexpr U8String& remove_suffix(view_type suffix);
		constexpr U8String& remove_suffix(UTF8Seq seq);

		constexpr U8String RemovePrefix(view_type prefix) const;
		constexpr U8String RemovePrefix(UTF8Seq seq) const;
		constexpr U8String RemoveSuffix(view_type suffix) const;
		constexpr U8String RemoveSuffix(UTF8Seq seq) const;

		//==================> partition <==================

		constexpr std::array<view_type, 3> partition(view_type delimiter) const;
		constexpr std::array<view_type, 3> partition(UTF8Seq delimiter) const;

		//==================> trim <==================

		constexpr U8String& trim(view_type characters = u8" \t");
		constexpr U8String& trim_start(view_type characters = u8" \t");
		constexpr U8String& trim_end(view_type characters = u8" \t");
		constexpr U8String& trim(UTF8Seq seq);
		constexpr U8String& trim_start(UTF8Seq seq);
		constexpr U8String& trim_end(UTF8Seq seq);
		constexpr U8String& trim_invalid();
		constexpr U8String& trim_invalid_start();
		constexpr U8String& trim_invalid_end();

		constexpr U8String Trim(view_type characters = u8" \t");
		constexpr U8String TrimStart(view_type characters = u8" \t");
		constexpr U8String TrimEnd(view_type characters = u8" \t");
		constexpr U8String Trim(UTF8Seq seq);
		constexpr U8String TrimStart(UTF8Seq seq);
		constexpr U8String TrimEnd(UTF8Seq seq);
		constexpr U8String TrimInvalid();
		constexpr U8String TrimInvalidStart();
		constexpr U8String TrimInvalidEnd();

		//==================> split <==================

		template<container::CanAdd<view_type> Buffer>
		size_type split(Buffer& out, view_type delimiter, bool cull_empty = false, size_type limit = npos) const {
			return view().split(out, delimiter, cull_empty, limit);
		}

		template<std::invocable<view_type> F>
		size_type split_each(F&& func, view_type delimiter, bool cull_empty = false, size_type limit = npos) const {
			return view().split(std::forward<F>(func), delimiter, cull_empty, limit);
		}

		template<container::CanAdd<view_type> Buffer>
		size_type split(Buffer& out, UTF8Seq delimiter, bool cull_empty = false, size_type limit = npos) const {
			return view().split(out, delimiter, cull_empty, limit);
		}

		template<std::invocable<view_type> F>
		size_type split_each(F&& func, UTF8Seq delimiter, bool cull_empty = false, size_type limit = npos) const {
			return view().split(std::forward<F>(func), delimiter, cull_empty, limit);
		}

	protected:
		constexpr void _reset() noexcept;
		constexpr void _set_size(size_type value) noexcept;

		template<typename Getter, typename Fn>
		constexpr void _reserve(Getter&& getter, size_type count, Fn&& func);

		template<typename Fn>
		constexpr void _insert(size_type index, size_type len, Fn&& func);

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
}

namespace auxiliary
{
	template<typename Traits, typename Allocator, typename ContainerPolicy, size_t SSOSize>
	struct Hash<unicode::U8String<Traits, Allocator, ContainerPolicy, SSOSize>> {
		constexpr size_t operator()(const unicode::U8String<Traits, Allocator, ContainerPolicy, SSOSize>& str) const {
			return XXHash::xxhash(str);
		}
	};
}

#include "auxiliary/unicode/implement/u8string.inl"
