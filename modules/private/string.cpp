#include "pch.hpp"

#include <u8lib/string.hpp>

#include <memory>

namespace u8lib
{
	constexpr size_t make_aligned(size_t value, size_t alignment) {
		return (value - 1) / alignment * alignment + alignment;
	}

	struct policy_type {
		static constexpr size_t get_grow(size_t expect_size) {
			constexpr size_t at_least_grow = 16;
			const auto ret = make_aligned(expect_size / 2 + expect_size, sizeof(size_t)) + at_least_grow;

			// overflow
			assert(expect_size < ret);

			return ret;
		}

		static constexpr size_t get_reserve(size_t expect_size) {
			return make_aligned(expect_size, sizeof(size_t));
		}
	};

	struct StringHelper : u8string::allocator_type {
		using size_type = u8string::size_type;
		using pointer = u8string::pointer;
		using traits_type = u8string::traits_type;
		using allocator_type = u8string::allocator_type;

		StringHelper(u8string* ptr) : str(ptr) {}

		u8string* str;

		void reset() const noexcept {
			std::memset(str->buffer_, 0, u8string::SSOBufferSize);
			str->sso_flag_ = 1;
		}

		void set_size(size_type value) const noexcept {
			if (str->is_sso()) {
				str->sso_data_[value] = 0;
				str->sso_size_ = value;
			} else {
				str->data_[value] = 0;
				str->size_ = value;
			}
		}

		void destroy() {
			deallocate(str->data_, str->capacity_ + 1);
		}

		template<typename Getter, typename Fn>
		void reserve(Getter&& getter, size_type count, Fn&& func) {
			if (count > str->capacity()) {
				auto new_sz = std::forward<Getter>(getter)(count + 1);
				pointer new_memory = allocator_type::allocate(new_sz);
				std::forward<Fn>(func)(new_memory);

				if (str->is_heap()) {
					deallocate(str->data(), str->capacity_ + 1);
				}

				str->data_ = new_memory;
				str->capacity_ = new_sz - 1;
				str->sso_flag_ = 0;
			}
		}

		template<typename Fn>
		void insert(size_type index, size_type len, Fn&& func) {
			const auto at_least_capacity = str->size() + len;
			if (at_least_capacity > str->capacity()) {
				const auto new_sz = policy_type::get_grow(at_least_capacity + 1);
				pointer new_memory = allocate(new_sz);
				traits_type::move(new_memory, str->data(), index);
				traits_type::move(new_memory + index + len, str->data() + index, str->size() - index + 1);
				std::forward<Fn>(func)(new_memory + index);

				if (str->is_heap()) {
					deallocate(str->data(), str->capacity_ + 1);
				}

				str->data_ = new_memory;
				str->capacity_ = new_sz - 1;
				str->sso_flag_ = 0;
			} else {
				traits_type::move(str->data() + index + len, str->data() + index, str->size() - index + 1);
				std::forward<Fn>(func)(str->data() + index);
			}
			set_size(at_least_capacity);
		}

		template<typename Char>
		u8string& do_assign(const Char* ptr, size_t len) {
			size_type utf8_len = u8lib::text_size(ptr, len);
			this->reserve(policy_type::get_reserve, utf8_len, [](pointer) {});
			u8lib::parse_to_utf8(ptr, len, str->data());
			set_size(utf8_len);
			return *str;
		}

		template<typename View>
		u8string& do_insert(size_type index, View view) {
			assert(index <= str->size());

			auto utf8_len = u8lib::text_size(view.data(), view.size());
			this->insert(index, utf8_len, [&](pointer ptr) {
				u8lib::parse_to_utf8(view.data(), view.size(), ptr);
			});
			return *str;
		}

		template<typename View>
		u8string& do_append(View view) {
			auto sz = str->size();
			size_type new_sz = sz + u8lib::text_size(view.data(), view.size());
			this->reserve(policy_type::get_grow, new_sz, [&](pointer ptr) {
				traits_type::move(ptr, str->data(), sz);
			});
			u8lib::parse_to_utf8(view.data(), view.size(), str->data() + sz);
			set_size(new_sz);
			return *str;
		}
	};
}

// ctor & dtor
namespace u8lib
{
	u8string::u8string() {
		StringHelper helper(this);
		helper.reset();
	}

	u8string::u8string(size_type count, value_type ch) {
		StringHelper helper(this);
		helper.reserve(policy_type::get_reserve, count, [](pointer) {});
		std::uninitialized_fill_n(data(), count, ch);
		helper.set_size(count);
	}

	u8string::u8string(u8string&& rhs) noexcept {
		StringHelper helper(this);
		helper.reset();
		swap(rhs);
	}

	u8string::u8string(u8string_view view) {
		StringHelper helper(this);
		helper.reset();
		assign(view.data(), view.size());
	}

	u8string::u8string(const char16_t* str, size_type count) {
		StringHelper helper(this);
		helper.reset();
		assign(str, count);
	}

	u8string::u8string(const char32_t* str, size_type count) {
		StringHelper helper(this);
		helper.reset();
		assign(str, count);
	}

	u8string::~u8string() noexcept {
		if (is_heap()) {
			StringHelper helper(this);
			helper.destroy();
		}
	}
}

// assign
namespace u8lib
{
	u8string& u8string::assign(u8string&& rhs) noexcept {
		StringHelper helper(this);
		if (is_heap()) helper.destroy();
		helper.reset();
		swap(rhs);
		return *this;
	}

	u8string& u8string::assign(u8string_view view) {
		StringHelper helper(this);
		return helper.do_assign(view.data(), view.size());
	}

	u8string& u8string::assign(const char16_t* str, size_type count) {
		StringHelper helper(this);
		return helper.do_assign(str, count);
	}

	u8string& u8string::assign(const char32_t* str, size_type count) {
		StringHelper helper(this);
		return helper.do_assign(str, count);
	}
}

// insert
namespace u8lib
{
	u8string& u8string::insert(size_type index, size_type count, value_type ch) {
		assert(index <= size());

		StringHelper helper(this);
		helper.insert(index, count, [&](pointer ptr) {
			std::uninitialized_fill_n(ptr, count, ch);
		});
		return *this;
	}

	u8string& u8string::insert(size_type index, u8string_view view) {
		StringHelper helper(this);
		return helper.do_insert(index, view);
	}

	u8string& u8string::insert(size_type index, const char16_t* str) {
		StringHelper helper(this);
		return helper.do_insert(index, std::basic_string_view{str});
	}

	u8string& u8string::insert(size_type index, const char32_t* str) {
		StringHelper helper(this);
		return helper.do_insert(index, std::basic_string_view{str});
	}

	u8string& u8string::append(size_type count, value_type ch) {
		StringHelper helper(this);

		auto sz = size();
		size_type new_sz = sz + count;
		helper.reserve(policy_type::get_grow, new_sz, [&](pointer ptr) {
			traits_type::move(ptr, data(), sz);
		});
		std::uninitialized_fill_n(data() + sz, count, ch);
		helper.set_size(new_sz);
		return *this;
	}

	u8string& u8string::append(u8string_view view) {
		StringHelper helper(this);
		return helper.do_append(view);
	}

	u8string& u8string::append(const char16_t* str) {
		StringHelper helper(this);
		return helper.do_append(std::basic_string_view{str});
	}

	u8string& u8string::append(const char32_t* str) {
		StringHelper helper(this);
		return helper.do_append(std::basic_string_view{str});
	}
}

// remove
namespace u8lib
{
	u8string& u8string::clear() noexcept {
		StringHelper helper(this);
		helper.set_size(0);
		return *this;
	}

	u8string& u8string::pop_back(size_type count) {
		assert(size() >= count);

		StringHelper helper(this);
		helper.set_size(size() - count);
		return *this;
	}

	u8string& u8string::erase(size_type index, size_type count) {
		assert(is_valid_index(index));

		StringHelper helper(this);
		auto v_end = std::min(count, size() - index);
		traits_type::move(data() + index, data() + index + v_end, size() - index - v_end);
		helper.set_size(size() - v_end);
		return *this;
	}
}

// replace
namespace u8lib
{
	u8string& u8string::replace(size_type pos, size_type count, const_pointer cstr, size_type count2) {
		assert(pos + count <= size());

		StringHelper helper(this);
		auto at_least_capacity = size() + count2 - count;
		if (at_least_capacity > capacity()) {
			auto new_sz = policy_type::get_grow(at_least_capacity + 1);
			auto new_memory = helper.allocate(new_sz);
			traits_type::move(new_memory, data(), pos);
			traits_type::copy(new_memory + pos, cstr, count2);
			traits_type::move(new_memory + pos + count2, data() + pos + count, size() - pos - count);

			if (is_heap()) {
				helper.destroy();
			}

			data_ = new_memory;
			capacity_ = new_sz - 1;
			sso_flag_ = 0;
		} else {
			traits_type::move(data() + pos + count2, data() + pos + count, size() - pos - count);
			traits_type::copy(data() + pos, cstr, count2);
		}

		helper.set_size(at_least_capacity);
		return *this;
	}
}

// misc
namespace u8lib
{
	void u8string::reserve(size_type new_cap) {
		StringHelper helper(this);
		helper.reserve(policy_type::get_reserve, new_cap, [&](pointer ptr) {
			// '\0'
			traits_type::move(ptr, data(), size() + 1);
		});
	}

	void u8string::resize(size_type count) {
		StringHelper helper(this);

		const auto sz = size();
		reserve(count);
		if (sz < count) {
			std::uninitialized_default_construct_n(data() + sz, count);
		}
		helper.set_size(count);
	}

	void u8string::resize(size_type count, value_type ch) {
		StringHelper helper(this);

		const auto sz = size();
		reserve(count);
		if (sz < count) {
			std::uninitialized_fill_n(data() + sz, count, ch);
		}
		helper.set_size(count);
	}
}
