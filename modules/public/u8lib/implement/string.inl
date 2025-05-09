#pragma once

namespace u8lib::internal
{
	using const_wchar_ptr = std::conditional_t<sizeof(wchar_t) == sizeof(char16_t), const char16_t*, const char32_t*>;
}

// join
namespace u8lib
{
	template<typename... Args>
	u8string u8string::concat(Args&&... string_or_view) {
		std::array<u8string_view, sizeof...(Args)> args{u8string_view{string_or_view}...};

		// calc size
		size_type total_size = 0;
		for (const auto& arg: args) {
			total_size += arg.size();
		}

		// combine
		u8string result;
		result.reserve(total_size);
		for (const auto& arg: args) {
			result.append(arg);
		}

		return result;
	}

	template<typename Container>
	u8string u8string::join(const Container& container, u8string_view separator, bool skip_empty, u8string_view trim_chs) {
		// calc size
		size_type total_size = 0;
		bool is_first_append = false;
		for (const auto& str: container) {
			u8string_view view{str};

			// trim
			if (!trim_chs.empty()) {
				view = view.trim(trim_chs);
			}

			// skip empty
			if (skip_empty && view.empty()) continue;

			// append separator
			if (!is_first_append) {
				is_first_append = true;
			} else {
				total_size += separator.size();
			}

			// append item
			total_size += view.size();
		}

		// combine
		u8string result;
		result.reserve(total_size);
		is_first_append = false;
		for (const auto& str: container) {
			u8string_view view{str};

			// trim
			if (!trim_chs.empty()) {
				view = view.trim(trim_chs);
			}

			// skip empty
			if (skip_empty && view.empty()) continue;

			// append separator
			if (!is_first_append) {
				is_first_append = true;
			} else {
				result.append(separator);
			}

			// append item
			result.append(view);
		}

		assert(result.size() == total_size && "Join failed");

		return result;
	}
}

// ctor & dtor
namespace u8lib
{
	inline u8string::u8string(const u8string& other): u8string(other.data(), other.size()) {}

	inline u8string::u8string(const char* str): u8string(u8string_view{str}) {}
	inline u8string::u8string(const char8_t* str): u8string(u8string_view{str}) {}
	inline u8string::u8string(const char16_t* str): u8string(str, std::char_traits<char16_t>::length(str)) {}
	inline u8string::u8string(const char32_t* str): u8string(str, std::char_traits<char32_t>::length(str)) {}
	inline u8string::u8string(const wchar_t* str): u8string(str, std::char_traits<wchar_t>::length(str)) {}

	inline u8string::u8string(const char* str, size_type count): u8string(u8string_view{str, count}) {}
	inline u8string::u8string(const char8_t* str, size_type count): u8string(u8string_view{str, count}) {}
	inline u8string::u8string(const wchar_t* str, size_type count): u8string(reinterpret_cast<internal::const_wchar_ptr>(str), count) {}

	inline u8string::u8string(u8string_view view, size_type pos): u8string(view.data() + pos, view.size() - pos) {}
	inline u8string::u8string(u8string_view view, size_type pos, size_type count): u8string(view.data() + pos, count) {}
}

// assign
namespace u8lib
{
	inline u8string& u8string::assign(u8string_view view, size_type pos, size_type count) { return assign(view.subview(pos, count)); }

	inline u8string& u8string::assign(const char* str) { return assign(u8string_view{str}); }
	inline u8string& u8string::assign(const char8_t* str) { return assign(u8string_view{str}); }
	inline u8string& u8string::assign(const char16_t* str) { return assign(str, std::char_traits<char16_t>::length(str)); }
	inline u8string& u8string::assign(const char32_t* str) { return assign(str, std::char_traits<char32_t>::length(str)); }
	inline u8string& u8string::assign(const wchar_t* str) { return assign(str, std::char_traits<wchar_t>::length(str)); }

	inline u8string& u8string::assign(const char* str, size_type count) { return assign(u8string_view{str, count}); }
	inline u8string& u8string::assign(const char8_t* str, size_type count) { return assign(u8string_view{str, count}); }
	inline u8string& u8string::assign(const wchar_t* str, size_type count) { return assign(reinterpret_cast<internal::const_wchar_ptr>(str), count); }

	inline u8string& u8string::operator=(const u8string& rhs) { return assign(rhs); }
	inline u8string& u8string::operator=(u8string&& rhs) noexcept { return assign(std::move(rhs)); }
	inline u8string& u8string::operator=(u8string_view view) { return assign(view); }
	inline u8string& u8string::operator=(const char* str) { return assign(str); }
	inline u8string& u8string::operator=(const char8_t* str) { return assign(str); }
	inline u8string& u8string::operator=(const char16_t* str) { return assign(str); }
	inline u8string& u8string::operator=(const char32_t* str) { return assign(str); }
	inline u8string& u8string::operator=(const wchar_t* str) { return assign(str); }
}

// compare
namespace u8lib
{
	inline bool u8string::operator==(const char* str) const noexcept { return u8string_view(*this) == str; }
	inline bool u8string::operator==(const char8_t* str) const noexcept { return u8string_view(*this) == str; }
	inline bool u8string::operator==(u8string_view str) const noexcept { return u8string_view(*this) == str; }
	inline bool u8string::operator==(const u8string& str) const noexcept { return u8string_view(*this) == u8string_view(str); }
	inline std::strong_ordering u8string::operator<=>(const char* str) const noexcept { return u8string_view(*this) <=> str; }
	inline std::strong_ordering u8string::operator<=>(const char8_t* str) const noexcept { return u8string_view(*this) <=> str; }
	inline std::strong_ordering u8string::operator<=>(u8string_view str) const noexcept { return u8string_view(*this) <=> str; }
	inline std::strong_ordering u8string::operator<=>(const u8string& str) const noexcept { return u8string_view(*this) <=> u8string_view(str); }
}

// iterator
namespace u8lib
{
	inline u8string::pointer u8string::begin() noexcept { return data(); }
	inline u8string::const_pointer u8string::begin() const noexcept { return data(); }
	inline u8string::const_pointer u8string::cbegin() const noexcept { return data(); }

	inline u8string::pointer u8string::end() noexcept { return data() + size(); }
	inline u8string::const_pointer u8string::end() const noexcept { return data() + size(); }
	inline u8string::const_pointer u8string::cend() const noexcept { return data() + size(); }

	inline std::reverse_iterator<u8string::pointer> u8string::rbegin() noexcept { return std::reverse_iterator(end()); }
	inline std::reverse_iterator<u8string::const_pointer> u8string::rbegin() const noexcept { return std::reverse_iterator(end()); }
	inline std::reverse_iterator<u8string::const_pointer> u8string::crbegin() const noexcept { return std::reverse_iterator(end()); }

	inline std::reverse_iterator<u8string::pointer> u8string::rend() noexcept { return std::reverse_iterator(begin()); }
	inline std::reverse_iterator<u8string::const_pointer> u8string::rend() const noexcept { return std::reverse_iterator(begin()); }
	inline std::reverse_iterator<u8string::const_pointer> u8string::crend() const noexcept { return std::reverse_iterator(begin()); }

	inline u8string::cursor u8string::cursor_begin() { return cursor::Begin(data(), size()); }
	inline u8string::cursor u8string::cursor_end() { return cursor::End(data(), size()); }
	inline u8string::iterator u8string::iter() { return cursor::Begin(data(), size()).as_iter(); }
	inline u8string::reverse_iterator u8string::iter_inv() { return cursor::End(data(), size()).as_iter_inv(); }
	inline u8string::range_t u8string::range() { return cursor::Begin(data(), size()).as_range(); }
	inline u8string::reverse_range u8string::range_inv() { return cursor::End(data(), size()).as_range_inv(); }

	inline u8string::const_cursor u8string::cursor_begin() const { return const_cursor::Begin(data(), size()); }
	inline u8string::const_cursor u8string::cursor_end() const { return const_cursor::End(data(), size()); }
	inline u8string::const_iterator u8string::iter() const { return const_cursor::Begin(data(), size()).as_iter(); }
	inline u8string::const_reverse_iterator u8string::iter_inv() const { return const_cursor::End(data(), size()).as_iter_inv(); }
	inline u8string::const_range u8string::range() const { return const_cursor::Begin(data(), size()).as_range(); }
	inline u8string::const_reverse_range u8string::range_inv() const { return const_cursor::End(data(), size()).as_range_inv(); }
}

// size
namespace u8lib
{
	inline bool u8string::empty() const noexcept {
		return !size();
	}

	inline u8string::size_type u8string::length() const noexcept {
		return size();
	}

	inline u8string::size_type u8string::text_length() const noexcept {
		return u8string_view(*this).text_length();
	}

	inline u8string::size_type u8string::capacity() const noexcept {
		return is_sso() ? SSOCapacity : capacity_;
	}

	inline u8string::size_type u8string::slack() const noexcept {
		return capacity() - size();
	}

	inline u8string::size_type u8string::size() const noexcept {
		return is_sso() ? sso_size_ : size_;
	}

	inline u8string::size_type u8string::max_size() const noexcept {
		return u8string_view(*this).max_size();
	}

	template<is_char_v Char>
	u8string::size_type u8string::to_size() const noexcept {
		return u8string_view(*this).to_size<Char>();
	}
}

// access
namespace u8lib
{
	inline u8string::reference u8string::at(size_type pos) {
		assert(is_valid_index(pos));
		return data()[pos];
	}

	inline u8string::reference u8string::operator[](size_type pos) {
		return at(pos);
	}

	inline u8string::reference u8string::front() {
		return at(0);
	}

	inline u8string::reference u8string::back() {
		return at(size() - 1);
	}

	inline u8string::pointer u8string::data() noexcept {
		return is_sso() ? sso_data_ : data_;
	}

	inline u8string::const_reference u8string::at(size_type pos) const {
		assert(is_valid_index(pos));
		return data()[pos];
	}

	inline u8string::const_reference u8string::operator[](size_type pos) const {
		return at(pos);
	}

	inline u8string::const_reference u8string::front() const {
		return at(0);
	}

	inline u8string::const_reference u8string::back() const {
		return at(size() - 1);
	}

	inline u8string::const_pointer u8string::data() const noexcept {
		return is_sso() ? sso_data_ : data_;
	}

	inline u8string::raw_reference u8string::raw_at(size_type pos) {
		assert(is_valid_index(pos));
		return raw_data()[pos];
	}

	inline u8string::raw_reference u8string::raw_front() {
		return raw_at(0);
	}

	inline u8string::raw_reference u8string::raw_back() {
		return raw_at(size() - 1);
	}

	inline u8string::raw_pointer u8string::raw_data() noexcept {
		return reinterpret_cast<raw_pointer>(data());
	}

	inline u8string::raw_const_reference u8string::raw_at(size_type pos) const {
		assert(is_valid_index(pos));
		return raw_data()[pos];
	}

	inline u8string::raw_const_reference u8string::raw_front() const {
		return raw_at(0);
	}

	inline u8string::raw_const_reference u8string::raw_back() const {
		return raw_at(size() - 1);
	}

	inline u8string::raw_const_pointer u8string::raw_data() const noexcept {
		return reinterpret_cast<raw_const_pointer>(data());
	}

	inline u8string::const_pointer u8string::c_str() const noexcept {
		return data();
	}

	inline UTF8Seq u8string::at_text(size_type index) const {
		return u8string_view(*this).at_text(index);
	}

	inline UTF8Seq u8string::last_text(size_type index) const {
		return u8string_view(*this).last_text(index);
	}

	inline bool u8string::is_sso() const noexcept {
		return sso_flag_;
	}

	inline bool u8string::is_heap() const noexcept {
		return !sso_flag_;
	}

	inline bool u8string::is_valid_index(size_type index) const noexcept {
		return u8string_view(*this).is_valid_index(index);
	}

	inline u8string::size_type u8string::buffer_index_to_text(size_type index) const noexcept {
		return u8string_view(*this).buffer_index_to_text(index);
	}

	inline u8string::size_type u8string::text_index_to_buffer(size_type index) const noexcept {
		return u8string_view(*this).text_index_to_buffer(index);
	}
}

// substr
namespace u8lib
{
	inline u8string::operator u8string_view() const noexcept {
		return {data(), size()};
	}

	inline u8string_view u8string::first_view(size_type count) const {
		return u8string_view(*this).first_view(count);
	}

	inline u8string_view u8string::last_view(size_type count) const {
		return u8string_view(*this).last_view(count);
	}

	inline u8string_view u8string::subview(size_type start, size_type count) const noexcept {
		return u8string_view(*this).subview(start, count);
	}

	inline u8string u8string::first_str(size_type count) const {
		return u8string{first_view(count)};
	}

	inline u8string u8string::last_str(size_type count) const {
		return u8string{last_view(count)};
	}

	inline u8string u8string::substr(size_type pos, size_type count) const {
		return u8string{subview(pos, count)};
	}
}

// add
namespace u8lib
{
	inline u8string& u8string::insert(size_type index, UTF8Seq seq) {
		assert(index <= size());
		if (seq.is_valid()) {
			return insert(index, u8string_view{seq.data, seq.len});
		}
		return *this;
	}

	inline u8string& u8string::insert(size_type index, const wchar_t* str) {
		return insert(index, reinterpret_cast<internal::const_wchar_ptr>(str));
	}

	inline u8string& u8string::append(UTF8Seq seq) {
		if (seq.is_valid()) {
			return append(u8string_view(seq.data, seq.len));
		}
		return *this;
	}

	inline u8string& u8string::append(const wchar_t* str) {
		return append(reinterpret_cast<internal::const_wchar_ptr>(str));
	}

	inline u8string& u8string::push_back(value_type ch) {
		return append(1, ch);
	}

	inline void u8string::operator+=(value_type ch) {
		push_back(ch);
	}

	inline void u8string::operator+=(UTF8Seq seq) {
		append(seq);
	}

	inline void u8string::operator+=(u8string_view view) {
		append(view);
	}

	inline void u8string::operator+=(const char16_t* str) {
		append(str);
	}

	inline void u8string::operator+=(const char32_t* str) {
		append(str);
	}

	inline void u8string::operator+=(const wchar_t* str) {
		append(str);
	}
}

// replace
namespace u8lib
{
	inline u8string& u8string::replace(size_type pos, size_type count, u8string_view view) {
		return replace(pos, count, view.data(), view.size());
	}

	inline u8string u8string::Replace(size_type pos, size_type count, const_pointer cstr, size_type count2) const {
		return u8string(*this).replace(pos, count, cstr, count2);
	}

	inline u8string u8string::Replace(size_type pos, size_type count, u8string_view view) const {
		return u8string(*this).replace(pos, count, view);
	}
}

// starts with
namespace u8lib
{
	inline bool u8string::starts_with(u8string_view sv) const noexcept {
		return u8string_view(*this).starts_with(sv);
	}

	inline bool u8string::starts_with(value_type ch) const noexcept {
		return u8string_view(*this).starts_with(ch);
	}

	inline bool u8string::starts_with(UTF8Seq seq) const {
		return u8string_view(*this).starts_with(seq);
	}

	inline bool u8string::ends_with(u8string_view sv) const noexcept {
		return u8string_view(*this).ends_with(sv);
	}

	inline bool u8string::ends_with(value_type ch) const noexcept {
		return u8string_view(*this).ends_with(ch);
	}

	inline bool u8string::ends_with(UTF8Seq seq) const {
		return u8string_view(*this).ends_with(seq);
	}
}

// contains & count
namespace u8lib
{
	inline bool u8string::contains(u8string_view sv) const noexcept {
		return u8string_view(*this).contains(sv);
	}

	inline bool u8string::contains(value_type ch) const noexcept {
		return u8string_view(*this).contains(ch);
	}

	inline bool u8string::contains(UTF8Seq seq) const {
		return u8string_view(*this).contains(seq);
	}

	inline u8string::size_type u8string::count(u8string_view pattern) const {
		return u8string_view(*this).count(pattern);
	}

	inline u8string::size_type u8string::count(UTF8Seq seq) const {
		return u8string_view(*this).count(seq);
	}
}

// find
namespace u8lib
{
#define U8LIB_FIND(name)	\
	inline u8string::data_reference u8string::name(u8string_view v, size_type pos) noexcept { return u8string_view(*this).name(v, pos); }	\
	inline u8string::data_reference u8string::name(value_type ch, size_type pos) noexcept { return u8string_view(*this).name(ch, pos); }	\
	inline u8string::data_reference u8string::name(UTF8Seq seq, size_type pos) { return u8string_view(*this).name(seq, pos); }	\
	inline u8string::data_reference u8string::name(const_pointer s, size_type pos, size_type count) { return u8string_view(*this).name(s, pos, count); }	\
	inline u8string::const_data_reference u8string::name(u8string_view v, size_type pos) const noexcept { return u8string_view(*this).name(v, pos); }	\
	inline u8string::const_data_reference u8string::name(value_type ch, size_type pos) const noexcept { return u8string_view(*this).name(ch, pos); }	\
	inline u8string::const_data_reference u8string::name(UTF8Seq seq, size_type pos) const { return u8string_view(*this).name(seq, pos); }	\
	inline u8string::const_data_reference u8string::name(const_pointer s, size_type pos, size_type count) const { return u8string_view(*this).name(s, pos, count); }
	U8LIB_FIND(find)
	U8LIB_FIND(find_first_of)
	U8LIB_FIND(find_first_not_of)
	U8LIB_FIND(rfind)
	U8LIB_FIND(find_last_of)
	U8LIB_FIND(find_last_not_of)

#undef U8LIB_FIND
}

// remove prefix & prefix
namespace u8lib
{
	inline u8string& u8string::remove_prefix(size_type n) {
		return erase(0, n);
	}

	inline u8string& u8string::remove_prefix(u8string_view prefix) {
		if (starts_with(prefix)) {
			return erase(0, prefix.size());
		}
		return *this;
	}

	inline u8string& u8string::remove_prefix(UTF8Seq prefix) {
		if (prefix.is_valid()) {
			return remove_prefix(u8string_view(prefix.data, prefix.len));
		}
		return *this;
	}

	inline u8string& u8string::remove_suffix(size_type n) {
		return erase(size() - n);
	}

	inline u8string& u8string::remove_suffix(u8string_view suffix) {
		if (ends_with(suffix)) {
			return erase(size() - suffix.size());
		}
		return *this;
	}

	inline u8string& u8string::remove_suffix(UTF8Seq suffix) {
		if (suffix.is_valid()) {
			return remove_suffix(u8string_view(suffix.data, suffix.len));
		}
		return *this;
	}

	inline u8string u8string::RemovePrefix(size_type n) const {
		return u8string{u8string_view(*this).RemovePrefix(n)};
	}

	inline u8string u8string::RemovePrefix(u8string_view prefix) const {
		return u8string{u8string_view(*this).RemovePrefix(prefix)};
	}

	inline u8string u8string::RemovePrefix(UTF8Seq prefix) const {
		return u8string{u8string_view(*this).RemovePrefix(prefix)};
	}

	inline u8string u8string::RemoveSuffix(size_type n) const {
		return u8string{u8string_view(*this).RemoveSuffix(n)};
	}

	inline u8string u8string::RemoveSuffix(u8string_view suffix) const {
		return u8string{u8string_view(*this).RemoveSuffix(suffix)};
	}

	inline u8string u8string::RemoveSuffix(UTF8Seq suffix) const {
		return u8string{u8string_view(*this).RemoveSuffix(suffix)};
	}
}

// partition
namespace u8lib
{
	inline std::array<u8string_view, 3> u8string::partition(u8string_view delimiter) const {
		return u8string_view(*this).partition(delimiter);
	}

	inline std::array<u8string_view, 3> u8string::partition(UTF8Seq delimiter) const {
		return u8string_view(*this).partition(delimiter);
	}
}

// trim
namespace u8lib
{
	inline u8string& u8string::trim(u8string_view characters) {
		return trim_start(characters).trim_end(characters);
	}

	inline u8string& u8string::trim_start(u8string_view characters) {
		auto target = u8string_view(*this).trim_start(characters);
		auto count = size() - target.size();
		return count ? erase(0, count) : *this;
	}

	inline u8string& u8string::trim_end(u8string_view characters) {
		auto target = u8string_view(*this).trim_end(characters);
		return target.size() == size() ? *this : erase(target.size());
	}

	inline u8string& u8string::trim(UTF8Seq seq) {
		if (seq.is_valid()) {
			return trim(u8string_view(seq.data, seq.len));
		}
		return *this;
	}

	inline u8string& u8string::trim_start(UTF8Seq seq) {
		if (seq.is_valid()) {
			return trim_start(u8string_view(seq.data, seq.len));
		}
		return *this;
	}

	inline u8string& u8string::trim_end(UTF8Seq seq) {
		if (seq.is_valid()) {
			return trim_end(u8string_view(seq.data, seq.len));
		}
		return *this;
	}

	inline u8string& u8string::trim_invalid() {
		return trim_invalid_start().trim_invalid_end();
	}

	inline u8string& u8string::trim_invalid_start() {
		auto target = u8string_view(*this).trim_invalid_start();
		auto count = size() - target.size();
		return count ? erase(0, count) : *this;
	}

	inline u8string& u8string::trim_invalid_end() {
		auto target = u8string_view(*this).trim_invalid_end();
		return target.size() == size() ? *this : erase(target.size());
	}

	inline u8string u8string::Trim(u8string_view characters) const {
		return u8string{u8string_view(*this).trim(characters)};
	}

	inline u8string u8string::TrimStart(u8string_view characters) const {
		return u8string{u8string_view(*this).trim_start(characters)};
	}

	inline u8string u8string::TrimEnd(u8string_view characters) const {
		return u8string{u8string_view(*this).trim_end(characters)};
	}

	inline u8string u8string::Trim(UTF8Seq seq) const {
		return u8string{u8string_view(*this).trim(seq)};
	}

	inline u8string u8string::TrimStart(UTF8Seq seq) const {
		return u8string{u8string_view(*this).trim_start(seq)};
	}

	inline u8string u8string::TrimEnd(UTF8Seq seq) const {
		return u8string{u8string_view(*this).trim_end(seq)};
	}

	inline u8string u8string::TrimInvalid() const {
		return u8string{u8string_view(*this).trim_invalid()};
	}

	inline u8string u8string::TrimInvalidStart() const {
		return u8string{u8string_view(*this).trim_invalid_start()};
	}

	inline u8string u8string::TrimInvalidEnd() const {
		return u8string{u8string_view(*this).trim_invalid_end()};
	}
}

// split
namespace u8lib
{
	template<internal::CanAdd<u8string_view> Buffer>
	u8string::size_type u8string::split(Buffer& out, u8string_view delimiter, bool cull_empty, size_type limit) const {
		return u8string_view(*this).split(out, delimiter, cull_empty, limit);
	}

	template<std::invocable<u8string_view> F>
	u8string::size_type u8string::split_each(F&& func, u8string_view delimiter, bool cull_empty, size_type limit) const {
		return u8string_view(*this).split(std::forward<F>(func), delimiter, cull_empty, limit);
	}

	template<internal::CanAdd<u8string_view> Buffer>
	u8string::size_type u8string::split(Buffer& out, UTF8Seq delimiter, bool cull_empty, size_type limit) const {
		return u8string_view(*this).split(out, delimiter, cull_empty, limit);
	}

	template<std::invocable<u8string_view> F>
	u8string::size_type u8string::split_each(F&& func, UTF8Seq delimiter, bool cull_empty, size_type limit) const {
		return u8string_view(*this).split(std::forward<F>(func), delimiter, cull_empty, limit);
	}
}

// misc
namespace u8lib
{
	inline void u8string::release(size_type reserve_capacity) {
		reserve(reserve_capacity);
		clear();
	}

	inline void u8string::swap(u8string& other) noexcept {
		std::swap(buffer_, other.buffer_);
	}

	inline u8string& u8string::reverse(size_type start, size_type count) {
		assert(is_valid_index(start) && "undefined behaviour accessing out of bounds");
		assert(count == npos || count <= size() - start && "undefined behaviour exceeding size of string view");
		count = count == npos ? size() - start : count;

		std::reverse(data() + start, data() + start + count);
		return *this;
	}

	inline u8string::size_type u8string::copy(pointer dest, size_type count, size_type pos) const {
		return u8string_view(*this).copy(dest, count, pos);
	}
}
