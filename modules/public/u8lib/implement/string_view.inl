#pragma once

// ctor & dtor
namespace u8lib
{
	constexpr u8string_view::u8string_view() noexcept : data_() {}
	constexpr u8string_view::u8string_view(const_pointer str) : data_(str) {}
	constexpr u8string_view::u8string_view(const_pointer str, size_type count): data_(str, count) {}
	inline u8string_view::u8string_view(raw_const_pointer str) : data_(reinterpret_cast<const_pointer>(str)) {}
	inline u8string_view::u8string_view(raw_const_pointer str, size_type count) : data_(reinterpret_cast<const_pointer>(str), count) {}
}

// compare
namespace u8lib
{
	constexpr bool u8string_view::operator==(const u8string_view& rhs) const noexcept {
		return data_ == rhs.data_;
	}

	constexpr std::strong_ordering u8string_view::operator<=>(const u8string_view& rhs) const noexcept {
		return data_ <=> rhs.data_;
	}

	constexpr int u8string_view::compare(u8string_view v) const noexcept {
		return data_.compare(v.data_);
	}

	constexpr int u8string_view::compare(size_type pos1, size_type count1, u8string_view v) const {
		return data_.compare(pos1, count1, v.data_);
	}

	constexpr int u8string_view::compare(size_type pos1, size_type count1, u8string_view v, size_type pos2, size_type count2) const {
		return data_.compare(pos1, count1, v.data_, pos2, count2);
	}

	constexpr int u8string_view::compare(size_type pos1, size_type count1, const_pointer s, size_type count2) const {
		return data_.compare(pos1, count1, s, count2);
	}
}

// iterator
namespace u8lib
{
	constexpr u8string_view::const_pointer u8string_view::begin() const noexcept { return data(); }
	constexpr u8string_view::const_pointer u8string_view::cbegin() const noexcept { return data(); }
	constexpr u8string_view::const_pointer u8string_view::end() const noexcept { return data() + size(); }
	constexpr u8string_view::const_pointer u8string_view::cend() const noexcept { return data() + size(); }
	constexpr std::reverse_iterator<u8string_view::const_pointer> u8string_view::rbegin() const noexcept { return std::reverse_iterator{end()}; }
	constexpr std::reverse_iterator<u8string_view::const_pointer> u8string_view::crbegin() const noexcept { return std::reverse_iterator{cend()}; }
	constexpr std::reverse_iterator<u8string_view::const_pointer> u8string_view::rend() const noexcept { return std::reverse_iterator{begin()}; }
	constexpr std::reverse_iterator<u8string_view::const_pointer> u8string_view::crend() const noexcept { return std::reverse_iterator{cbegin()}; }

	inline u8string_view::raw_const_pointer u8string_view::raw_begin() const noexcept { return raw_data(); }
	inline u8string_view::raw_const_pointer u8string_view::raw_cbegin() const noexcept { return raw_data(); }
	inline u8string_view::raw_const_pointer u8string_view::raw_end() const noexcept { return raw_data() + size(); }
	inline u8string_view::raw_const_pointer u8string_view::raw_cend() const noexcept { return raw_data() + size(); }
	inline std::reverse_iterator<u8string_view::raw_const_pointer> u8string_view::raw_rbegin() const noexcept { return std::reverse_iterator{raw_end()}; }
	inline std::reverse_iterator<u8string_view::raw_const_pointer> u8string_view::raw_crbegin() const noexcept { return std::reverse_iterator{raw_cend()}; }
	inline std::reverse_iterator<u8string_view::raw_const_pointer> u8string_view::raw_rend() const noexcept { return std::reverse_iterator{raw_begin()}; }
	inline std::reverse_iterator<u8string_view::raw_const_pointer> u8string_view::raw_crend() const noexcept { return std::reverse_iterator{raw_cbegin()}; }

	constexpr u8string_view::const_cursor u8string_view::cursor_begin() const { return const_cursor::Begin(data(), size()); }
	constexpr u8string_view::const_cursor u8string_view::cursor_end() const { return const_cursor::End(data(), size()); }
	constexpr u8string_view::const_iterator u8string_view::iter() const { return const_cursor::Begin(data(), size()).as_iter(); }
	constexpr u8string_view::const_reverse_iterator u8string_view::iter_inv() const { return const_cursor::End(data(), size()).as_iter_inv(); }
	constexpr u8string_view::const_range u8string_view::range() const { return const_cursor::Begin(data(), size()).as_range(); }
	constexpr u8string_view::const_reverse_range u8string_view::range_inv() const { return const_cursor::End(data(), size()).as_range_inv(); }
}

// size
namespace u8lib
{
	constexpr bool u8string_view::empty() const noexcept { return data_.empty(); }
	constexpr u8string_view::size_type u8string_view::length() const noexcept { return data_.length(); }
	constexpr u8string_view::size_type u8string_view::text_length() const noexcept { return empty() ? 0 : utf8_code_point_index(data(), size(), size() - 1) + 1; }
	constexpr u8string_view::size_type u8string_view::size() const noexcept { return data_.size(); }
	constexpr u8string_view::size_type u8string_view::max_size() const noexcept { return data_.max_size(); }

	template<is_char_v Char>
	constexpr u8string_view::size_type u8string_view::to_size() const noexcept {
		if constexpr (sizeof(Char) == sizeof(char8_t)) {
			return size();
		} else if constexpr (sizeof(Char) == sizeof(char16_t)) {
			if (empty()) {
				return 0;
			}

			using Cursor = UTF8Cursor<true>;

			size_type utf16_len = 0;
			for (UTF8Seq utf8_seq: Cursor{data(), size(), 0}.as_range()) {
				if (utf8_seq.is_valid()) {
					utf16_len += utf8_seq.to_utf16_len();
				} else {
					utf16_len += 1;
				}
			}

			return utf16_len;
		} else if constexpr (sizeof(Char) == sizeof(char32_t)) {
			if (empty()) {
				return 0;
			}

			using Cursor = UTF8Cursor<true>;
			size_type utf32_len = 0;
			for ([[maybe_unused]] UTF8Seq utf8_seq: Cursor{data(), size(), 0}.as_range()) {
				utf32_len += 1;
			}
			return utf32_len;
		} else {
			std::unreachable();
		}
	}
}

// access
namespace u8lib
{
	constexpr u8string_view::const_reference u8string_view::at(size_type pos) const { return data_.at(pos); }
	constexpr u8string_view::const_reference u8string_view::front() const { return data_.front(); }
	constexpr u8string_view::const_reference u8string_view::back() const { return data_.back(); }
	constexpr u8string_view::const_pointer u8string_view::data() const noexcept { return data_.data(); }
	constexpr std::u8string_view u8string_view::view() const noexcept { return data_; }

	inline u8string_view::raw_const_reference u8string_view::raw_at(size_type pos) const { return *reinterpret_cast<raw_const_pointer>(&at(pos)); }
	inline u8string_view::raw_const_reference u8string_view::raw_front() const { return *reinterpret_cast<raw_const_pointer>(&front()); }
	inline u8string_view::raw_const_reference u8string_view::raw_back() const { return *reinterpret_cast<raw_const_pointer>(&back()); }
	inline u8string_view::raw_const_pointer u8string_view::raw_data() const noexcept { return reinterpret_cast<raw_const_pointer>(data()); }
	inline std::string_view u8string_view::raw_view() const noexcept { return *(std::string_view*) &data_; }

	constexpr u8string_view::const_reference u8string_view::operator[](size_type pos) const { return data_[pos]; }

	constexpr UTF8Seq u8string_view::at_text(size_type pos) const {
		assert(this->is_valid_index(pos) && "undefined behavior accessing an empty string view");
		uint64_t seq_index;
		return UTF8Seq::ParseUTF8(data(), size(), pos, seq_index);
	}

	constexpr UTF8Seq u8string_view::last_text(size_type index) const {
		return at_text(size() - index - 1);
	}

	constexpr bool u8string_view::is_valid_index(size_type index) const noexcept {
		return index < size();
	}

	constexpr u8string_view::size_type u8string_view::buffer_index_to_text(size_type index) const noexcept {
		return utf8_code_point_index(data(), size(), index);
	}

	constexpr u8string_view::size_type u8string_view::text_index_to_buffer(size_type index) const noexcept {
		return utf8_code_unit_index(data(), size(), index);
	}
}

// remove prefix & prefix
namespace u8lib
{
	constexpr void u8string_view::remove_prefix(size_type n) {
		data_.remove_prefix(n);
	}

	constexpr void u8string_view::remove_prefix(u8string_view prefix) {
		if (starts_with(prefix)) remove_prefix(prefix.size());
	}

	constexpr void u8string_view::remove_prefix(UTF8Seq prefix) {
		if (prefix.is_valid()) {
			remove_prefix(u8string_view{prefix.data, prefix.len});
		}
	}

	constexpr u8string_view u8string_view::RemovePrefix(size_type n) const {
		return subview(n);
	}

	constexpr u8string_view u8string_view::RemovePrefix(u8string_view prefix) const {
		return starts_with(prefix) ? RemovePrefix(prefix.size()) : *this;
	}

	constexpr u8string_view u8string_view::RemovePrefix(UTF8Seq prefix) const {
		return prefix.is_valid() ? RemovePrefix(u8string_view{prefix.data, prefix.len}) : *this;
	}

	constexpr void u8string_view::remove_suffix(size_type n) {
		data_.remove_suffix(n);
	}

	constexpr void u8string_view::remove_suffix(u8string_view suffix) {
		if (ends_with(suffix)) remove_suffix(suffix.size());
	}

	constexpr void u8string_view::remove_suffix(UTF8Seq suffix) {
		if (suffix.is_valid()) {
			remove_suffix(u8string_view{suffix.data, suffix.len});
		}
	}

	constexpr u8string_view u8string_view::RemoveSuffix(size_type n) const {
		return subview(0, size() - n);
	}

	constexpr u8string_view u8string_view::RemoveSuffix(u8string_view suffix) const {
		return ends_with(suffix) ? RemoveSuffix(suffix.size()) : *this;
	}

	constexpr u8string_view u8string_view::RemoveSuffix(UTF8Seq suffix) const {
		return suffix.is_valid() ? RemoveSuffix(u8string_view{suffix.data, suffix.len}) : *this;
	}
}

// subview
namespace u8lib
{
	constexpr u8string_view u8string_view::first_view(size_type count) const {
		assert(count <= size() && "undefined behavior accessing out of bounds");
		return {data(), count};
	}

	constexpr u8string_view u8string_view::last_view(size_type count) const {
		assert(count <= size() && "undefined behavior accessing out of bounds");
		return {data() + size() - count, count};
	}

	constexpr u8string_view u8string_view::subview(size_type pos, size_type count) const {
		auto sv = data_.substr(pos, count);
		return {sv.data(), sv.size()};
	}
}

// starts & ends with
namespace u8lib
{
	constexpr bool u8string_view::starts_with(u8string_view sv) const noexcept {
		return data_.starts_with(sv.data_);
	}

	constexpr bool u8string_view::starts_with(value_type ch) const noexcept {
		return data_.starts_with(ch);
	}

	constexpr bool u8string_view::starts_with(UTF8Seq prefix) const {
		if (prefix.is_valid()) {
			return starts_with(u8string_view{prefix.data, prefix.len});
		}
		return false;
	}

	constexpr bool u8string_view::ends_with(u8string_view sv) const noexcept {
		return data_.ends_with(sv.data_);
	}

	constexpr bool u8string_view::ends_with(value_type ch) const noexcept {
		return data_.ends_with(ch);
	}

	constexpr bool u8string_view::ends_with(UTF8Seq suffix) const {
		if (suffix.is_valid()) {
			return ends_with(u8string_view{suffix.data, suffix.len});
		}
		return false;
	}
}

// contains & count
namespace u8lib
{
	constexpr bool u8string_view::contains(u8string_view sv) const noexcept {
		return data_.contains(sv.data_);
	}

	constexpr bool u8string_view::contains(value_type ch) const noexcept {
		return data_.contains(ch);
	}

	constexpr bool u8string_view::contains(UTF8Seq seq) const {
		if (seq.is_valid()) {
			return contains(u8string_view{seq.data, seq.len});
		}
		return false;
	}

	constexpr u8string_view::size_type u8string_view::count(u8string_view pattern) const {
		auto find_view{*this};
		size_type result = 0;
		while (true) {
			if (const auto find_result = find_view.find(pattern)) {
				++result;
				find_view = find_view.subview(find_result + pattern.size());
			} else {
				break;
			}
		}
		return result;
	}

	constexpr u8string_view::size_type u8string_view::count(UTF8Seq seq) const {
		if (seq.is_valid()) {
			return count(u8string_view{seq.data, seq.len});
		}
		return 0;
	}
}

// find
namespace u8lib
{
#define U8LIB_FIND(name)	\
	constexpr u8string_view::const_data_reference u8string_view::name(u8string_view v, size_type pos) const noexcept {	\
		if (const auto index = data_.name(v.data_, pos); index != npos) {	\
			return {data(), index};	\
		}	\
		return {};	\
	}	\
	constexpr u8string_view::const_data_reference u8string_view::name(value_type ch, size_type pos) const noexcept {	\
		if (const auto index = data_.name(ch, pos); index != npos) {	\
			return {data(), index};	\
		}	\
		return {};	\
	}	\
	constexpr u8string_view::const_data_reference u8string_view::name(const_pointer s, size_type pos, size_type count) const {	\
		if (const auto index = data_.name(s, pos, count); index != npos) {	\
			return {data(), index};	\
		}	\
		return {};	\
	}	\
	constexpr u8string_view::const_data_reference u8string_view::name(UTF8Seq pattern, size_type pos) const {	\
		if (pattern.is_valid()) {	\
			return name(u8string_view{pattern.data, pattern.len}, pos);	\
		}	\
		return {};\
	}

	U8LIB_FIND(find)
	U8LIB_FIND(find_first_of)
	U8LIB_FIND(find_first_not_of)
	U8LIB_FIND(rfind)
	U8LIB_FIND(find_last_of)
	U8LIB_FIND(find_last_not_of)

#undef U8LIB_FIND
}

// partition
namespace u8lib
{
	constexpr std::array<u8string_view, 3> u8string_view::partition(u8string_view delimiter) const {
		if (auto found = find(delimiter)) {
			return {
				subview(0, found),
				subview(found, delimiter.size()),
				subview(found + delimiter.size())
			};
		}
		return {*this, {}, {}};
	}

	constexpr std::array<u8string_view, 3> u8string_view::partition(UTF8Seq delimiter) const {
		if (delimiter.is_valid()) {
			return partition(u8string_view{delimiter.data, delimiter.len});
		}
		return {*this, {}, {}};
	}
}

// trim
namespace u8lib
{
	constexpr u8string_view u8string_view::trim(u8string_view characters) const {
		return trim_start(characters).trim_end(characters);
	}

	constexpr u8string_view u8string_view::trim_start(u8string_view characters) const {
		if (empty()) {
			return {};
		}

		if (characters.empty()) {
			return {*this};
		}

		for (auto cursor = cursor_begin(); !cursor.reach_end(); cursor.move_next()) {
			if (!characters.contains(cursor.ref())) {
				return subview(cursor.index());
			}
		}
		return {};
	}

	constexpr u8string_view u8string_view::trim_end(u8string_view characters) const {
		if (empty()) {
			return {};
		}
		if (characters.empty()) {
			return {*this};
		}

		for (auto cursor = cursor_end(); !cursor.reach_begin(); cursor.move_prev()) {
			if (!characters.contains(cursor.ref())) {
				return subview(0, cursor.index() + cursor.seq_len());
			}
		}
		return {};
	}

	constexpr u8string_view u8string_view::trim(UTF8Seq seq) const {
		if (seq.is_valid()) {
			return trim(u8string_view{seq.data, seq.len});
		}
		return *this;
	}

	constexpr u8string_view u8string_view::trim_start(UTF8Seq seq) const {
		if (seq.is_valid()) {
			return trim_start(u8string_view{seq.data, seq.len});
		}
		return *this;
	}

	constexpr u8string_view u8string_view::trim_end(UTF8Seq seq) const {
		if (seq.is_valid()) {
			return trim_end(u8string_view{seq.data, seq.len});
		}
		return *this;
	}

	constexpr u8string_view u8string_view::trim_invalid() const {
		return trim_invalid_start().trim_invalid_end();
	}

	constexpr u8string_view u8string_view::trim_invalid_start() const {
		if (empty()) {
			return {};
		}

		for (auto cursor = cursor_begin(); !cursor.reach_end(); cursor.move_next()) {
			if (cursor.ref().is_valid()) {
				return subview(cursor.index());
			}
		}
		return {};
	}

	constexpr u8string_view u8string_view::trim_invalid_end() const {
		if (empty()) {
			return {};
		}

		for (auto cursor = cursor_end(); !cursor.reach_begin(); cursor.move_prev()) {
			if (cursor.ref().is_valid()) {
				return subview(0, cursor.index() + cursor.seq_len());
			}
		}
		return {};
	}
}

// split
namespace u8lib
{
	template<internal::CanAdd<u8string_view> Buffer>
	constexpr u8string_view::size_type u8string_view::split(Buffer& out, u8string_view delimiter, bool cull_empty, size_type limit) const {
		u8string_view each_view{*this};
		size_type count = 0;
		while (true) {
			const auto [left, mid, right] = each_view.partition(delimiter);

			// append
			if (!cull_empty || !left.empty()) {
				++count;
				if constexpr (internal::HasAdd<Buffer, u8string_view>) {
					out.add(left);
				} else if constexpr (internal::HasAppend<Buffer, u8string_view>) {
					out.append(left);
				} else if constexpr (internal::HasEmplaceBack<Buffer, u8string_view>) {
					out.emplace_back(left);
				} else if constexpr (internal::HasPushBack<Buffer, u8string_view>) {
					out.push_back(left);
				} else {
					std::unreachable();
				}
			}

			// trigger end
			if (right.empty()) {
				break;
			}

			// limit break
			if (limit != npos && count >= limit) {
				break;
			}

			// update
			each_view = right;
		}
		return count;
	}

	template<internal::CanAdd<u8string_view> Buffer>
	constexpr u8string_view::size_type u8string_view::split(Buffer& out, UTF8Seq delimiter, bool cull_empty, size_type limit) const {
		return this->split(
			out,
			delimiter.is_valid() ? u8string_view{delimiter.data, delimiter.len} : u8string_view{},
			cull_empty,
			limit
		);
	}

	template<std::invocable<u8string_view> F>
	constexpr u8string_view::size_type u8string_view::split_each(F&& func, u8string_view delimiter, bool cull_empty, size_type limit) const {
		using EachFuncResultType = std::invoke_result_t<F, const u8string_view&>;

		u8string_view each_view{*this};
		size_type count = 0;
		while (true) {
			const auto [left, mid, right] = each_view.partition(delimiter);

			// append
			if (!cull_empty || !left.empty()) {
				++count;
				if constexpr (std::is_same_v<EachFuncResultType, bool>) {
					if (!func(left)) {
						break;
					}
				} else {
					func(left);
				}
			}

			// trigger end
			if (right.empty()) {
				break;
			}

			// limit break
			if (limit != npos && count >= limit) {
				break;
			}

			// update
			each_view = right;
		}
		return count;
	}

	template<std::invocable<u8string_view> F>
	constexpr u8string_view::size_type u8string_view::split_each(F&& func, UTF8Seq delimiter, bool cull_empty, size_type limit) const {
		return this->split_each(
			std::forward<F>(func),
			delimiter.is_valid() ? u8string_view{delimiter.data, delimiter.len} : u8string_view{},
			cull_empty,
			limit
		);
	}
}

// misc
namespace u8lib
{
	constexpr void u8string_view::swap(u8string_view& v) noexcept {
		data_.swap(v.data_);
	}

	constexpr u8string_view::size_type u8string_view::copy(pointer dest, size_type count, size_type pos) const {
		return data_.copy(dest, count, pos);
	}
}
