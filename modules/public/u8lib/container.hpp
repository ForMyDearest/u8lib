#pragma once

#include <type_traits>

namespace u8lib
{
	// Cursor -> Range
	template<typename TCursor, bool kInverse>
	struct CursorRange {
		struct EndType {};

		struct Adaptor {
			TCursor cursor;

			// compare
			constexpr bool operator!=(const EndType& rhs) const {
				if constexpr (kInverse) {
					return !cursor.reach_begin();
				} else {
					return !cursor.reach_end();
				}
			}

			// move
			constexpr Adaptor& operator++() {
				if constexpr (kInverse) {
					cursor.move_prev();
				} else {
					cursor.move_next();
				}
				return *this;
			}

			// dereference
			constexpr decltype(auto) operator*() { return cursor.ref(); }
		};

		constexpr CursorRange(TCursor&& cursor): _cursor(std::move(cursor)) {}
		constexpr CursorRange(const TCursor& cursor): _cursor(cursor) {}

		// begin & end
		constexpr Adaptor begin() { return {_cursor}; }
		constexpr EndType end() { return {}; }
		constexpr Adaptor begin() const { return {_cursor}; }
		constexpr EndType end() const { return {}; }

	private:
		TCursor _cursor;
	};

	// Cursor -> Iter
	template<typename TCursor, bool kInverse>
	struct CursorIter : protected TCursor {
		// ctor & copy & move & assign & move assign
		constexpr CursorIter(TCursor&& rhs): TCursor(std::move(rhs)) {}
		constexpr CursorIter(const TCursor& rhs): TCursor(rhs) {}

		constexpr CursorIter(const CursorIter& rhs) = default;
		constexpr CursorIter(CursorIter&& rhs) = default;
		constexpr CursorIter& operator=(const CursorIter& rhs) = default;
		constexpr CursorIter& operator=(CursorIter&& rhs) = default;

		// getter
		constexpr decltype(auto) ref() const { return TCursor::ref(); }

		// move & validator
		constexpr void reset() {
			if constexpr (kInverse) {
				TCursor::reset_to_end();
			} else {
				TCursor::reset_to_begin();
			}
		}

		constexpr void move_next() {
			if constexpr (kInverse) {
				TCursor::move_prev();
			} else {
				TCursor::move_next();
			}
		}

		constexpr bool has_next() const {
			if constexpr (kInverse) {
				return !TCursor::reach_begin();
			} else {
				return !TCursor::reach_end();
			}
		}

		// cast
		constexpr const TCursor& cursor() const { return *this; }
		constexpr TCursor& cursor() { return *this; }
		constexpr CursorRange<TCursor, kInverse> as_range() const { return {*this}; }
	};

	template<typename T, bool kConst>
	struct VectorDataRef {
		using value_type = std::conditional_t<kConst, const T, T>;
		using size_type = size_t;

		static constexpr size_type npos = static_cast<size_type>(-1);

		constexpr VectorDataRef() noexcept = default;
		constexpr VectorDataRef(value_type* ptr, size_type index) noexcept: data_(ptr), index_(index) {}

		template<bool kConstRHS>
		constexpr VectorDataRef(const VectorDataRef<T, kConstRHS>& rhs) noexcept: data_(const_cast<value_type*>(rhs.data_)), index_(rhs.index_) {}

		constexpr bool is_valid() const noexcept { return data_ != nullptr && index_ != npos; }
		constexpr value_type* ptr() const noexcept { return index_ == npos ? nullptr : data_ + index_; }
		constexpr value_type& ref() const noexcept { return *ptr(); }
		constexpr size_type index() const noexcept { return index_; }

		constexpr explicit operator bool() const noexcept { return is_valid(); }
		constexpr value_type& operator*() const noexcept { return ref(); }
		constexpr value_type* operator->() const noexcept { return ptr(); }
		constexpr operator size_type() const noexcept { return index_; }

		constexpr bool operator==(const VectorDataRef& rhs) const noexcept { return ptr() == rhs.ptr(); }
		constexpr bool operator!=(const VectorDataRef& rhs) const noexcept { return ptr() != rhs.ptr(); }

	private:
		template<typename, bool>
		friend struct VectorDataRef;

		value_type* data_ = nullptr;
		size_type index_ = npos;
	};

	namespace internal
	{
		template<typename T, typename Elem>
		concept HasAppend = requires(T t, Elem e)
		{
			t.append(e);
		};
		template<typename T, typename Elem>
		concept HasAdd = requires(T t, Elem e)
		{
			t.add(e);
		};
		template<typename T, typename Elem>
		concept HasEmplaceBack = requires(T t, Elem e)
		{
			t.emplace_back(e);
		};
		template<typename T, typename Elem>
		concept HasPushBack = requires(T t, Elem e)
		{
			t.push_back(e);
		};

		template<typename T, typename Elem>
		concept CanAdd = HasAppend<T, Elem> || HasAdd<T, Elem> || HasEmplaceBack<T, Elem> || HasPushBack<T, Elem>;
	}
}
