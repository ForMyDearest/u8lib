#pragma once

#include "u8lib/decode_utf.hpp"

namespace u8lib::internal
{
	U8LIB_API void report_error(const char8_t* message);
	U8LIB_API void vformat_to(buffer& buf, std::u8string_view fmt, format_args args, const void* loc = nullptr);
	U8LIB_API char8_t* vformat_to(char8_t* out, std::u8string_view fmt, format_args args, const void* loc = nullptr);
	U8LIB_API format_to_n_result<char8_t*> vformat_to_n(char8_t* out, size_t n, std::u8string_view fmt, format_args args, const void* loc = nullptr);
	U8LIB_API size_t vformatted_size(std::u8string_view fmt, format_args args, const void* loc = nullptr);

#pragma region Type

	static_assert(static_cast<int>(Type::custom_type) < 16, "must fit in 4-bit bitfield");

	constexpr bool is_arithmetic_fmt_type(const Type type) {
		return Type::none_type < type && type <= Type::last_numeric_type;
	}

	constexpr bool is_integral_fmt_type(const Type type) {
		return Type::none_type < type && type <= Type::last_integer_type;
	}

	// Maps core type T to the corresponding type enum constant.
	template<typename> struct type_constant : std::integral_constant<Type, Type::custom_type> {};

#define U8FMT_TYPE_CONSTANT(type, constant)\
	template<> struct type_constant<type> : std::integral_constant<Type, Type::constant> {}

	U8FMT_TYPE_CONSTANT(int, int_type);
	U8FMT_TYPE_CONSTANT(unsigned, uint_type);
	U8FMT_TYPE_CONSTANT(long long, long_long_type);
	U8FMT_TYPE_CONSTANT(unsigned long long, ulong_long_type);
	U8FMT_TYPE_CONSTANT(bool, bool_type);
	U8FMT_TYPE_CONSTANT(char8_t, char_type);
	U8FMT_TYPE_CONSTANT(float, float_type);
	U8FMT_TYPE_CONSTANT(double, double_type);
	U8FMT_TYPE_CONSTANT(long double, long_double_type);
	U8FMT_TYPE_CONSTANT(const void*, pointer_type);
	U8FMT_TYPE_CONSTANT(const char8_t*, cstring_type);
	U8FMT_TYPE_CONSTANT(std::u8string_view, string_type);

#undef U8FMT_TYPE_CONSTANT

	template<typename Arg, typename... Args>
	constexpr uint64_t encode_types() {
		uint64_t ret = static_cast<uint64_t>(type_constant<std::decay_t<Arg>>::value);
		if constexpr (sizeof...(Args) != 0) {
			ret |= (encode_types<Args...>() << packed_arg_bits);
		}
		return ret;
	}

	template<typename... T>
	constexpr uint64_t make_descriptor() {
		constexpr size_t NUM_ARGS = sizeof...(T);
		if constexpr (NUM_ARGS == 0) {
			return 0;
		} else if constexpr (NUM_ARGS <= max_packed_args) {
			return encode_types<T...>();
		} else {
			return is_unpacked_bit | NUM_ARGS;
		}
	}

#pragma endregion Type

#pragma region custom_value

	template<typename T>
	custom_value::custom_value(const T& value) noexcept {
		using CT = std::conditional_t<formattable<const T>, const T, T>;
		static_assert(formattable<CT>);

		ptr_ = std::addressof(value);
		format_ = [](parse_context& parse_ctx, context& format_ctx, const void* ptr) {
			using U = std::remove_const_t<T>;
			// doesn't drop const-qualifier per an unnumbered LWG issue
			formatter<U> formatter;
			parse_ctx.advance_to(formatter.parse(parse_ctx));
			format_ctx.advance_to(formatter.format(*const_cast<CT*>(static_cast<const U*>(ptr)), format_ctx));
		};
	}

	constexpr void custom_value::format(parse_context& parse_ctx, context& format_ctx) const {
		format_(parse_ctx, format_ctx, ptr_);
	}

#pragma endregion custom_value

#pragma region value

	constexpr value::value() noexcept: no_state_() {}
	constexpr value::value(int val) noexcept: int_state_(val) {}
	constexpr value::value(unsigned int val) noexcept: uint_state_(val) {}
	constexpr value::value(long long val) noexcept: long_long_state_(val) {}
	constexpr value::value(unsigned long long val) noexcept: ulong_long_state_(val) {}
	constexpr value::value(bool val) noexcept : bool_state_(val) {}
	constexpr value::value(char8_t val) noexcept : char_state_(val) {}
	constexpr value::value(float val) noexcept: float_state_(val) {}
	constexpr value::value(double val) noexcept : double_state_(val) {}
	constexpr value::value(long double val) noexcept: long_double_state_(val) {}
	constexpr value::value(const void* val) noexcept: pointer_state_(val) {}
	constexpr value::value(const char8_t* val) noexcept: cstring_state_(val) {}
	constexpr value::value(std::u8string_view val) noexcept: string_state_(val) {}
	template<typename T> constexpr value::value(const T& val) noexcept: custom_state_(val) {}

#pragma endregion value

#pragma region fstring

	// we need to implement this ourselves because from_chars does not work with wide characters and isn't constexpr
	constexpr const char8_t* parse_nonnegative_integer(const char8_t* first, const char8_t* last, unsigned int& value) {
		assert(first != last && '0' <= *first && *first <= '9');

		constexpr auto max_int = static_cast<unsigned int>(std::numeric_limits<int>::max());
		constexpr auto big_int = max_int / 10u;

		value = 0;

		do {
			if (value > big_int) {
				value = max_int + 1;
				break;
			}
			value = value * 10 + static_cast<unsigned int>(*first - '0');
			++first;
		} while (first != last && '0' <= *first && *first <= '9');

		if (value > max_int) {
			report_error(u8"number is too big");
		}

		return first;
	}

	constexpr const char8_t* parse_nonnegative_integer(const char8_t* first, const char8_t* last, int& value) {
		unsigned int unsigned_value = 0;

		first = parse_nonnegative_integer(first, last, unsigned_value);
		// Never invalid because _Parse_nonnegative_integer throws an error for values that don't fit in signed integers
		value = static_cast<int>(unsigned_value);
		return first;
	}

	template<typename Callbacks>
	constexpr const char8_t* parse_arg_id(const char8_t* first, const char8_t* last, Callbacks&& callbacks) {
		assert(first != last);

		const char8_t ch = *first;
		// No id provided, format string is using automatic indexing.
		if (ch == '}' || ch == ':') {
			callbacks.on_auto_id();
			return first;
		}

		if (ch >= '0' && ch <= '9') {
			unsigned int index = 0;
			// arg_id is not allowed to have any leading zeros, but is allowed to be
			// equal to zero (but not '00'). So if ch is zero we skip the parsing, leave
			// _Index set to zero and let the validity checks below ensure that the arg_id
			// wasn't something like "00", or "023".
			if (ch == '0') {
				++first;
			} else {
				first = parse_nonnegative_integer(first, last, index);
			}

			// The format string shouldn't end right after the index number.
			// The only things permitted after the index are the end of the replacement field ('}')
			// or the beginning of the format spec (':').
			if (first == last || (*first != '}' && *first != ':')) {
				report_error(u8"invalid format string.");
			}

			callbacks.on_manual_id(index);
			return first;
		}
		// This is where we would parse named arg ids if std::format were to support them.
		report_error(u8"invalid format string.");
		std::unreachable();
	}

	template<typename Handler>
	constexpr const char8_t* parse_replacement_field(const char8_t* first, const char8_t* last, Handler&& handler) {
		++first;
		if (first == last) {
			report_error(u8"invalid format string.");
		}

		if (*first == '}') {
			// string was "{}", and we have a replacement field
			handler.on_replacement_field(handler.parse_context_.next_arg_id(), first);
		} else if (*first == '{') {
			// string was "{{", so we have a literal "{" to print
			handler.on_text(first, first + 1);
		} else {
			// parse_arg_id expects a handler when it finds an argument id, however
			// parse_replacement_field actually needs to know the value of that argument ID to pass on
			// to Handler.on_replacement_field or Handler.on_format_specs. This parse_arg_id wrapper
			// stores the value of the arg id for later use, so parse_replacement_field has access to it.
			struct IdAdapter {
				parse_context& parse_context_;
				size_t arg_id_ = static_cast<size_t>(-1);

				constexpr void on_auto_id() {
					arg_id_ = parse_context_.next_arg_id();
					assert(arg_id_ != static_cast<size_t>(-1));
				}

				constexpr void on_manual_id(const size_t id) {
					parse_context_.check_arg_id(id);
					arg_id_ = id;
					assert(arg_id_ != static_cast<size_t>(-1));
				}
			} adapter = {handler.parse_context_};

			first = internal::parse_arg_id(first, last, adapter);
			char8_t ch{};
			if (first != last) {
				ch = *first;
			}

			if (ch == '}') {
				handler.on_replacement_field(adapter.arg_id_, first);
			} else if (ch == ':') {
				first = handler.on_format_specs(adapter.arg_id_, first + 1, last);
				if (first == last || *first != '}') {
					report_error(u8"unknown format specifier.");
				}
			} else {
				report_error(u8"missing '}' in format string.");
			}
		}

		return first + 1;
	}

	template<typename Handler>
	constexpr void parse_format_string(std::u8string_view fmt, Handler&& handler) {
		auto first = fmt.data();
		auto last = first + fmt.size();

		while (first != last) {
			const char8_t* openingCurl = first;
			if (*first != '{') {
				openingCurl = std::find(first, last, u8'{');

				for (;;) {
					const char8_t* closingCurl = std::find(first, openingCurl, u8'}');

					// In this case there are neither closing nor opening curls in [first, _OpeningCurl)
					// Write the whole thing out.
					if (closingCurl == openingCurl) {
						handler.on_text(first, openingCurl);
						break;
					}
					// We know _ClosingCurl isn't past the end because
					// the above condition was not met.
					++closingCurl;
					if (closingCurl == openingCurl || *closingCurl != '}') {
						report_error(u8"unmatched '}' in format string.");
					}
					// We found two closing curls, so output only one of them
					handler.on_text(first, closingCurl);

					// skip over the second closing curl
					first = closingCurl + 1;
				}

				// We are done, there were no replacement fields.
				if (openingCurl == last) {
					return;
				}
			}
			// Parse the replacement field starting at _OpeningCurl and ending sometime before last.
			first = internal::parse_replacement_field(openingCurl, last, handler);
		}
	}

	template<typename T>
	consteval parse_context::iterator compile_time_parse_format_specs(parse_context& pc) {
		using FormattedTypeMapping = format_arg_traits::storage_type<T>;
		// If the type is going to use a custom formatter we should just use that,
		// instead of trying to instantiate a custom formatter for its erased handle type
		using FormattedType = std::conditional_t<std::is_same_v<FormattedTypeMapping, custom_value>, T, FormattedTypeMapping>;
		return formatter<FormattedType>().parse(pc);
	}

	// set of format parsing actions that only checks for validity
	template<typename... Args>
	struct format_checker {
		using ParseFunc = parse_context::iterator (*)(parse_context&);

		static constexpr size_t NUM_ARGS = sizeof...(Args);
		parse_context parse_context_;
		ParseFunc parse_funcs_[NUM_ARGS > 0 ? NUM_ARGS : 1];

		consteval explicit format_checker(std::u8string_view fmt) noexcept
			: parse_context_(fmt, NUM_ARGS), parse_funcs_{&compile_time_parse_format_specs<Args>...} {}

		static constexpr void on_text(const char8_t*, const char8_t*) noexcept {}
		static constexpr void on_replacement_field(size_t, const char8_t*) noexcept {}

		constexpr const char8_t* on_format_specs(size_t id, const char8_t* first, const char8_t*) {
			parse_context_.advance_to(parse_context_.begin() + (first - std::to_address(parse_context_.begin())));
			if (id < NUM_ARGS) {
				auto iter = parse_funcs_[id](parse_context_); // TRANSITION, VSO-1451773 (workaround: named variable)
				return std::to_address(iter);
			}
			return first;
		}
	};

	template<typename... Args>
	template<std::convertible_to<std::u8string_view> T>
	consteval fstring<Args...>::fstring(const T& s) : str(s) {
		internal::parse_format_string(str, format_checker<std::remove_cvref_t<Args>...>{str});
	}

	template<typename... Args>
	constexpr std::u8string_view fstring<Args...>::get() const noexcept {
		return str;
	}

#pragma endregion fstring

#pragma region buffer

	class buffer {
		using grow_fun = void (*)(buffer* buf, size_t capacity);

	public:
		using value_type = char8_t;
		using const_reference = const char8_t&;

		constexpr buffer(buffer&&) = default;
		buffer(const buffer&) = delete;
		void operator=(const buffer&) = delete;

		constexpr char8_t& operator[](size_t index) { return ptr_[index]; }
		constexpr char8_t operator[](size_t index) const { return ptr_[index]; }

		constexpr char8_t* begin() noexcept { return ptr_; }
		constexpr const char8_t* begin() const noexcept { return ptr_; }

		constexpr char8_t* end() noexcept { return ptr_ + size_; }
		constexpr const char8_t* end() const noexcept { return ptr_ + size_; }

		constexpr char8_t* data() noexcept { return ptr_; }
		constexpr const char8_t* data() const noexcept { return ptr_; }

		constexpr size_t size() const noexcept { return size_; }
		constexpr size_t capacity() const noexcept { return capacity_; }

		constexpr void clear() {
			size_ = 0;
		}

		constexpr void try_resize(size_t count) {
			try_reserve(count);
			size_ = std::min(count, capacity_);
		}

		constexpr void try_reserve(size_t new_capacity) {
			if (new_capacity > capacity_) grow_(this, new_capacity);
		}

		constexpr void push_back(char8_t value) {
			try_reserve(size_ + 1);
			ptr_[size_++] = value;
		}

		constexpr void append(const char8_t* begin, const char8_t* end) {
			while (begin != end) {
				auto count = static_cast<size_t>(end - begin);
				try_reserve(size_ + count);
				auto free_cap = capacity_ - size_;
				if (free_cap < count) count = free_cap;
				// A loop is faster than memcpy on small sizes.
				char8_t* out = ptr_ + size_;
				for (size_t i = 0; i < count; ++i) out[i] = begin[i];
				size_ += count;
				begin += count;
			}
		}

	protected:
		constexpr buffer(grow_fun grow, size_t sz) noexcept
			: size_(sz), capacity_(sz), grow_(grow) {}

		constexpr explicit buffer(grow_fun grow, char8_t* p = nullptr, size_t sz = 0, size_t cap = 0) noexcept
			: ptr_(p), size_(sz), capacity_(cap), grow_(grow) {}

		constexpr void set(char8_t* buf_data, size_t buf_capacity) noexcept {
			ptr_ = buf_data;
			capacity_ = buf_capacity;
		}

		char8_t* ptr_;
		size_t size_;
		size_t capacity_;
		grow_fun grow_;
	};

	struct buffer_traits {
		constexpr explicit buffer_traits(size_t) {}
		static constexpr size_t count() { return 0; }
		static constexpr size_t limit(const size_t size) { return size; }
	};

	class fixed_buffer_traits {
	public:
		constexpr explicit fixed_buffer_traits(const size_t limit) : limit_(limit) {}
		constexpr size_t count() const { return count_; }

		constexpr size_t limit(const size_t size) {
			const size_t n = limit_ > count_ ? limit_ - count_ : 0;
			count_ += size;
			return std::min(size, n);
		}

	private:
		size_t count_ = 0;
		size_t limit_;
	};

	// A buffer that writes to an output iterator when flushed.
	template<typename OutputIt, typename Traits>
	class iterator_buffer : public Traits, public buffer {
	public:
		explicit constexpr iterator_buffer(OutputIt out, size_t n = buffer_size)
			: Traits(n), buffer(grow, data_, 0, buffer_size), out_(out) {}

		constexpr iterator_buffer(iterator_buffer&& other) noexcept
			: Traits(other), buffer(grow, data_, 0, buffer_size), out_(other.out_) {}

		constexpr ~iterator_buffer() {
			// Don't crash if flush fails during unwinding.
			try { flush(); } catch (...) {}
		}

		constexpr OutputIt out() {
			flush();
			return out_;
		}

		constexpr size_t count() const {
			return Traits::count() + this->size();
		}

	private:
		enum { buffer_size = 256 };
		char8_t data_[buffer_size] = {};
		OutputIt out_;

		static constexpr void grow(buffer* buf, size_t) {
			if (buf->size() == buffer_size) reinterpret_cast<iterator_buffer*>(buf)->flush();
		}

		constexpr void flush() {
			auto size = this->size();
			this->clear();
			const char8_t* begin = data_;
			const char8_t* end = begin + this->limit(size);
			while (begin != end) *out_++ = *begin++;
		}
	};

#pragma endregion buffer

#pragma region appender

	constexpr appender::appender(buffer& buf): container(&buf) {}

	constexpr appender& appender::operator=(char8_t c) {
		container->push_back(c);
		return *this;
	}

	constexpr appender& appender::operator*() { return *this; }
	constexpr appender& appender::operator++() { return *this; }
	constexpr appender appender::operator++(int) const { return *this; }

#pragma endregion appender

#pragma region specs_setter

	constexpr specs_setter::specs_setter(basic_format_specs& specs): specs_(specs) {}
	constexpr void specs_setter::on_align(Align aln) const { specs_.alignment_ = aln; }
	constexpr void specs_setter::on_sign(Sign sgn) const { specs_.sgn_ = sgn; }
	constexpr void specs_setter::on_hash() const { specs_.alt_ = true; }
	constexpr void specs_setter::on_zero() const { specs_.leading_zero_ = true; }
	constexpr void specs_setter::on_width(int width) const { specs_.width_ = width; }
	constexpr void specs_setter::on_precision(int precision) const { specs_.precision_ = precision; }
	constexpr void specs_setter::on_localized() const { specs_.localized_ = true; }
	constexpr void specs_setter::on_type(char8_t type) const { specs_.type_ = type; }

	constexpr void specs_setter::on_fill(std::u8string_view sv) const {
		if (sv.size() > std::size(specs_.fill_)) {
			report_error(u8"invalid fill (too long).");
		}

		const auto pos = std::copy(sv.begin(), sv.end(), specs_.fill_);
		std::fill(pos, std::end(specs_.fill_), char8_t{});
		specs_.fill_length_ = static_cast<uint8_t>(sv.size());
	}

#pragma endregion specs_setter

#pragma region dynamic_specs_handler

	constexpr dynamic_specs_handler::dynamic_specs_handler(dynamic_format_specs& specs, parse_context& parse_ctx)
		: specs_setter(specs), dynamic_specs_(specs), parse_ctx_(parse_ctx) {}

	constexpr void dynamic_specs_handler::on_dynamic_width(size_t arg_id) const {
		parse_ctx_.check_arg_id(arg_id);
		dynamic_specs_.dynamic_width_index_ = verify_dynamic_arg_index_in_range(arg_id);
	}

	constexpr void dynamic_specs_handler::on_dynamic_width(auto_id_tag) const {
		dynamic_specs_.dynamic_width_index_ = verify_dynamic_arg_index_in_range(parse_ctx_.next_arg_id());
	}

	constexpr void dynamic_specs_handler::on_dynamic_precision(size_t arg_id) const {
		parse_ctx_.check_arg_id(arg_id);
		dynamic_specs_.dynamic_precision_index_ = verify_dynamic_arg_index_in_range(arg_id);
	}

	constexpr void dynamic_specs_handler::on_dynamic_precision(auto_id_tag) const {
		dynamic_specs_.dynamic_precision_index_ = verify_dynamic_arg_index_in_range(parse_ctx_.next_arg_id());
	}

	constexpr int dynamic_specs_handler::verify_dynamic_arg_index_in_range(size_t idx) {
		if (idx > static_cast<size_t>(std::numeric_limits<int>::max())) {
			report_error(u8"dynamic width or precision index too large.");
		}

		return static_cast<int>(idx);
	}

#pragma endregion dynamic_specs_handler

#pragma region compile_parse

	// Checks that the type of the argument printed by a replacement
	// field with format specs actually satisfies the requirements for
	// that format spec. If the requirements are met then calls the base class
	// handler method.
	template<typename Handler>
	class specs_checker : public Handler {
	public:
		constexpr explicit specs_checker(const Handler& handler_inst, Type arg_type)
			: Handler(handler_inst), arg_type_(arg_type) {}

		constexpr void require_numeric_argument() const {
			if (!is_arithmetic_fmt_type(arg_type_)) {
				report_error(u8"format specifier requires numeric argument.");
			}
		}

		constexpr void check_precision() const {
			if (is_integral_fmt_type(arg_type_) || arg_type_ == Type::pointer_type) {
				report_error(u8"precision not allowed for this argument type.");
			}
		}

		constexpr void on_localized() {
			require_numeric_argument();
			Handler::on_localized();
		}

		constexpr void on_hash() {
			need_arithmetic_presentation_type_ = true;
			require_numeric_argument();
			Handler::on_hash();
		}

		constexpr void on_sign(Sign sgn) {
			need_arithmetic_presentation_type_ = true;
			require_numeric_argument();
			Handler::on_sign(sgn);
		}

		constexpr void on_zero() {
			need_arithmetic_presentation_type_ = true;
			require_numeric_argument();
			Handler::on_zero();
		}

		constexpr void on_precision(int precision) {
			check_precision();
			Handler::on_precision(precision);
		}

		template<typename T>
		constexpr void on_dynamic_precision(T value) {
			check_precision();
			Handler::on_dynamic_precision(value);
		}

		constexpr void on_type(char8_t type) {
			if (type < 0 || type > std::numeric_limits<signed char>::max()) {
				report_error(u8"invalid type specification.");
			}
			const char narrow_type = static_cast<char>(type);
			enum class Presentation_type_category {
				Default,
				Integer,
				Floating,
				String,
				Pointer,
				Char,
				Escape,
			};
			auto catagory = Presentation_type_category::Default;
			switch (narrow_type) {
				case '\0':
					break;
				case '?':
					catagory = Presentation_type_category::Escape;
					break;
				case 's':
					catagory = Presentation_type_category::String;
					break;
				case 'c':
					catagory = Presentation_type_category::Char;
					break;
				case 'd':
				case 'B':
				case 'b':
				case 'X':
				case 'x':
				case 'o':
					catagory = Presentation_type_category::Integer;
					break;
				case 'A':
				case 'a':
				case 'E':
				case 'e':
				case 'F':
				case 'f':
				case 'G':
				case 'g':
					catagory = Presentation_type_category::Floating;
					break;
				case 'p':
					catagory = Presentation_type_category::Pointer;
					break;
				default:
					report_error(u8"invalid presentation type specifier");
			}

			switch (arg_type_) {
				case Type::none_type:
					report_error(u8"invalid argument type.");
					break;
				case Type::bool_type:
					if (catagory == Presentation_type_category::Default) {
						catagory = Presentation_type_category::String;
					}
				// note, we don't get a call if there isn't a type, but none is valid for everything.
					if (catagory != Presentation_type_category::String && catagory != Presentation_type_category::Integer) {
						report_error(u8"invalid presentation type for bool");
					}
					break;
				case Type::char_type:
					if (catagory == Presentation_type_category::Default) {
						catagory = Presentation_type_category::Char;
					}

					if (catagory != Presentation_type_category::Char && catagory != Presentation_type_category::Integer && catagory != Presentation_type_category::Escape) {
						report_error(u8"invalid presentation type for char8_t");
					}
					break;
				case Type::int_type:
				case Type::uint_type:
				case Type::long_long_type:
				case Type::ulong_long_type:
					if (catagory == Presentation_type_category::Default) {
						catagory = Presentation_type_category::Integer;
					}

					if (catagory != Presentation_type_category::Integer && catagory != Presentation_type_category::Char) {
						report_error(u8"invalid presentation type for integer");
					}
					break;
				case Type::float_type:
				case Type::double_type:
				case Type::long_double_type:
					if (catagory == Presentation_type_category::Default) {
						catagory = Presentation_type_category::Floating;
					}

					if (catagory != Presentation_type_category::Floating) {
						report_error(u8"invalid presentation type for floating-point");
					}
					break;
				case Type::cstring_type:
				case Type::string_type:
					if (catagory == Presentation_type_category::Default) {
						catagory = Presentation_type_category::String;
					}

					if (catagory != Presentation_type_category::String && catagory != Presentation_type_category::Escape) {
						report_error(u8"invalid presentation type for string");
					}
					break;
				case Type::pointer_type:
					if (catagory == Presentation_type_category::Default) {
						catagory = Presentation_type_category::Pointer;
					}

					if (catagory != Presentation_type_category::Pointer) {
						report_error(u8"invalid presentation type for pointer");
					}
					break;
				case Type::custom_type:
					// there's no checking we can do here for custom types
					// (however if a custom type uses a standard formatter
					// to do its spec parsing it should get the above checks)
					break;
			}

			if (need_arithmetic_presentation_type_ && catagory
				!=
				Presentation_type_category::Integer && catagory != Presentation_type_category::Floating
			) {
				report_error(u8"modifier requires an integer presentation type for bool");
			}
			Handler::on_type(type);
		}

	private:
		Type arg_type_;
		// we'll see this if we get a modifier that requires an integer presentation type
		// for types that can have either integer or non-integer presentation types (charT or bool)
		bool need_arithmetic_presentation_type_ = false;
	};

	template<typename Callbacks>
	constexpr const char8_t* parse_align(const char8_t* first, const char8_t* last, Callbacks&& callbacks) {
		// align and fill
		assert(first != last && *first != '}');
		auto parsed_align = Align::none;

		const int units = utf8_code_units_in_next_character(first, last);
		if (units < 0) {
			// invalid fill character encoding
			report_error(u8"invalid format string.");
		}
		auto align_pt = first + units;

		if (align_pt == last) {
			align_pt = first;
		}

		for (;;) {
			switch (*align_pt) {
				case '<':
					parsed_align = Align::left;
					break;
				case '>':
					parsed_align = Align::right;
					break;
				case '^':
					parsed_align = Align::center;
					break;
				default:
					break;
			}

			if (parsed_align != Align::none) {
				if (align_pt != first) {
					if (*first == '{') {
						report_error(u8"invalid fill character '{'");
					}
					callbacks.on_fill({first, static_cast<size_t>(align_pt - first)});
					first = align_pt + 1;
				} else {
					++first;
				}
				callbacks.on_align(parsed_align);
				break;
			}
			if (align_pt == first) {
				break;
			}
			align_pt = first;
		}

		return first;
	}

	template<typename Callbacks>
	constexpr const char8_t* parse_width(const char8_t* first, const char8_t* last, Callbacks&& callbacks) {
		assert(first != last);
		if ('1' <= *first && *first <= '9') {
			int value = 0;
			first = internal::parse_nonnegative_integer(first, last, value);
			callbacks.on_width(value);
		} else if (*first == '{') {
			++first;
			if (first != last) {
				// Adapts a type modeling _Width_adapter_callbacks to model _Parse_arg_id_callbacks.
				// Used in _Parse_width so that _Parse_arg_id can be used to parse dynamic widths.
				struct width_adapter {
					Callbacks& callbacks_;

					constexpr explicit width_adapter(Callbacks& handler) : callbacks_(handler) {}

					constexpr void on_auto_id() {
						callbacks_.on_dynamic_width(auto_id_tag{});
					}

					constexpr void on_manual_id(const size_t id) {
						callbacks_.on_dynamic_width(id);
					}
				} adapter{callbacks};

				first = internal::parse_arg_id(first, last, adapter);
			}

			if (first == last || *first != '}') {
				report_error(u8"invalid format string.");
			}
			++first;
		}
		return first;
	}

	template<typename Callbacks>
	constexpr const char8_t* parse_precision(const char8_t* first, const char8_t* last, Callbacks&& callbacks) {
		++first;
		char8_t ch = '\0';
		if (first != last) {
			ch = *first;
		}

		if ('0' <= ch && ch <= '9') {
			int precision = 0;
			first = parse_nonnegative_integer(first, last, precision);
			callbacks.on_precision(precision);
		} else if (ch == '{') {
			++first;
			if (first != last) {
				// Adapts a type modeling precision_adapter_callbacks to model parse_arg_id_callbacks.
				// Used in parse_precision so that parse_arg_id can be used to parse dynamic precisions.
				struct precision_adapter {
					Callbacks& callbacks_;

					constexpr explicit precision_adapter(Callbacks& handler) : callbacks_(handler) {}

					constexpr void on_auto_id() {
						callbacks_.on_dynamic_precision(auto_id_tag{});
					}

					constexpr void on_manual_id(const size_t id) {
						callbacks_.on_dynamic_precision(id);
					}
				} adapter{callbacks};

				first = internal::parse_arg_id(first, last, adapter);
			}

			if (first == last || *first != '}') {
				report_error(u8"invalid format string.");
			}
			++first;
		} else {
			report_error(u8"missing precision specifier.");
		}

		return first;
	}

	template<typename Callbacks>
	constexpr const char8_t* parse_format_specs(const char8_t* first, const char8_t* last, Callbacks&& callbacks) {
		if (first == last || *first == '}') {
			return first;
		}

		first = internal::parse_align(first, last, callbacks);
		if (first == last) {
			return first;
		}

		switch (*first) {
			case '+':
				callbacks.on_sign(Sign::plus);
				++first;
				break;
			case '-':
				callbacks.on_sign(Sign::minus);
				++first;
				break;
			case ' ':
				callbacks.on_sign(Sign::space);
				++first;
				break;
			default:
				break;
		}

		if (first == last) {
			return first;
		}

		if (*first == '#') {
			callbacks.on_hash();
			if (++first == last) {
				return first;
			}
		}

		if (*first == '0') {
			callbacks.on_zero();
			if (++first == last) {
				return first;
			}
		}

		first = internal::parse_width(first, last, callbacks);
		if (first == last) {
			return first;
		}

		if (*first == '.') {
			first = internal::parse_precision(first, last, callbacks);
			if (first == last) {
				return first;
			}
		}

		if (*first == 'L') {
			callbacks.on_localized();
			if (++first == last) {
				return first;
			}
		}

		// If there's anything remaining we assume it's a type.
		if (*first != '}') {
			callbacks.on_type(*first);
			++first;
		} else {
			// call the type callback so it gets a default type, this is required
			// since _Specs_checker needs to be able to tell that it got a default type
			// to raise an error for default formatted bools with a sign modifier
			callbacks.on_type(u8'\0');
		}

		return first;
	}

	template<typename T, Type ArgType>
	constexpr auto formatter_base<T, ArgType>::parse(parse_context& parse_ctx) {
		specs_checker handler(dynamic_specs_handler{specs_, parse_ctx}, ArgType);
		const auto iter = internal::parse_format_specs(parse_ctx.unchecked_begin(), parse_ctx.unchecked_end(), handler);
		if (iter != parse_ctx.unchecked_end() && *iter != '}') {
			report_error(u8"missing '}' in format string.");
		}
		return parse_ctx.begin() + (iter - parse_ctx.unchecked_begin());
	}

#pragma endregion compile_parse
}

namespace u8lib
{
#pragma region format_arg

	template<typename Visitor>
	constexpr decltype(auto) format_arg::visit(Visitor&& vis) {
		using enum internal::Type;
		switch (active_state_) {
			case none_type:
				return std::forward<Visitor>(vis)(value_.no_state_);
			case int_type:
				return std::forward<Visitor>(vis)(value_.int_state_);
			case uint_type:
				return std::forward<Visitor>(vis)(value_.uint_state_);
			case long_long_type:
				return std::forward<Visitor>(vis)(value_.long_long_state_);
			case ulong_long_type:
				return std::forward<Visitor>(vis)(value_.ulong_long_state_);
			case bool_type:
				return std::forward<Visitor>(vis)(value_.bool_state_);
			case char_type:
				return std::forward<Visitor>(vis)(value_.char_state_);
			case float_type:
				return std::forward<Visitor>(vis)(value_.float_state_);
			case double_type:
				return std::forward<Visitor>(vis)(value_.double_state_);
			case long_double_type:
				return std::forward<Visitor>(vis)(value_.long_double_state_);
			case pointer_type:
				return std::forward<Visitor>(vis)(value_.pointer_state_);
			case cstring_type:
				return std::forward<Visitor>(vis)(value_.cstring_state_);
			case string_type:
				return std::forward<Visitor>(vis)(value_.string_state_);
			case custom_type:
				return std::forward<Visitor>(vis)(value_.custom_state_);
			default:
				std::unreachable();
		}
	}

	constexpr format_arg::operator bool() const noexcept {
		return active_state_ != internal::Type::none_type;
	}

	constexpr format_arg::format_arg() noexcept: active_state_(internal::Type::none_type) {}
	constexpr format_arg::format_arg(int val) noexcept: value_(val), active_state_(internal::Type::int_type) {}
	constexpr format_arg::format_arg(unsigned int val) noexcept: value_(val), active_state_(internal::Type::uint_type) {}
	constexpr format_arg::format_arg(long long val) noexcept: value_(val), active_state_(internal::Type::long_long_type) {}
	constexpr format_arg::format_arg(unsigned long long val) noexcept: value_(val), active_state_(internal::Type::ulong_long_type) {}
	constexpr format_arg::format_arg(bool val) noexcept : value_(val), active_state_(internal::Type::bool_type) {}
	constexpr format_arg::format_arg(char8_t val) noexcept : value_(val), active_state_(internal::Type::char_type) {}
	constexpr format_arg::format_arg(float val) noexcept: value_(val), active_state_(internal::Type::float_type) {}
	constexpr format_arg::format_arg(double val) noexcept : value_(val), active_state_(internal::Type::double_type) {}
	constexpr format_arg::format_arg(long double val) noexcept: value_(val), active_state_(internal::Type::long_double_type) {}
	constexpr format_arg::format_arg(const void* val) noexcept: value_(val), active_state_(internal::Type::pointer_type) {}
	constexpr format_arg::format_arg(const char8_t* val) noexcept: value_(val), active_state_(internal::Type::cstring_type) {}
	constexpr format_arg::format_arg(std::u8string_view val) noexcept: value_(val), active_state_(internal::Type::string_type) {}
	template<typename T> constexpr format_arg::format_arg(const T& val) noexcept: value_(val), active_state_(internal::Type::custom_type) {}


#pragma endregion format_arg

#pragma region format_args

	constexpr format_args::format_args() noexcept: desc_(0), args_(nullptr) {}

	constexpr format_args::format_args(internal::format_arg_store<0>, size_t) noexcept : format_args() {}

	template<size_t NUM_ARGS>
	format_args::format_args(const internal::format_arg_store<NUM_ARGS>& s, size_t desc)
		: desc_(desc), values_(s.args) {}

#pragma endregion format_args

#pragma region parse_context

	constexpr parse_context::parse_context(std::u8string_view fmt, size_t num_args) noexcept
		: format_string_(fmt), num_args_(num_args) {}

	constexpr parse_context::const_iterator parse_context::begin() const noexcept {
		return format_string_.begin();
	}

	constexpr parse_context::const_iterator parse_context::end() const noexcept {
		return format_string_.end();
	}

	constexpr const char8_t* parse_context::unchecked_begin() const noexcept {
		return format_string_.data();
	}

	constexpr const char8_t* parse_context::unchecked_end() const noexcept {
		return format_string_.data() + format_string_.size();
	}

	constexpr void parse_context::advance_to(const const_iterator& iter) {
		assert(format_string_.begin() <= iter && iter <= format_string_.end());

		const auto diff = static_cast<size_t>(iter - format_string_.begin());
		format_string_.remove_prefix(diff);
	}

	constexpr size_t parse_context::next_arg_id() {
		if (next_arg_id_ < 0) {
			internal::report_error(u8"can not switch from manual to automatic indexing");
		}

		if (std::is_constant_evaluated()) {
			if (next_arg_id_ >= num_args_) internal::report_error(u8"argument not found");
		}

		return static_cast<size_t>(next_arg_id_++);
	}

	constexpr void parse_context::check_arg_id(size_t id) {
		if (std::is_constant_evaluated()) {
			if (id >= num_args_) internal::report_error(u8"argument not found");
		}

		if (next_arg_id_ > 0) {
			internal::report_error(u8"can not switch from automatic to manual indexing");
		}
		next_arg_id_ = -1;
	}

#pragma endregion parse_context

#pragma region context

	inline context::context(iterator iter, format_args ctx_args, const void* loc)
		: out_(iter), args_(ctx_args), loc_(loc) {}

	inline format_arg context::arg(size_t id) const noexcept {
		return args_.get(id);
	}

	inline context::iterator context::out() const {
		return out_;
	}

	inline void context::advance_to(iterator it) {
		out_ = it;
	}

	inline const format_args& context::get_args() const noexcept {
		return args_;
	}

	inline const void* context::get_lazy_locale() const {
		return loc_;
	}

#pragma endregion context

	template<typename... Args>
	constexpr auto make_format_store(Args&... args) noexcept {
		static_assert(
			(formattable<std::remove_cvref_t<Args>> && ...),
			"Cannot format an argument. To make type T formattable, provide a formatter<T> specialization. "
			"See N4950 [format.arg.store]/2 and [formatter.requirements]."
		);
		return internal::format_arg_store<sizeof...(Args)>{args...};
	}

	//=====================> format_to <========================

	template<std::output_iterator<const char8_t&> OutputIt, typename... T>
	OutputIt format_to(OutputIt out, format_string<T...> fmt, T&&... args) {
		constexpr auto DESC = internal::make_descriptor<T...>();
		return u8lib::vformat_to(out, fmt.get(), format_args(u8lib::make_format_store(args...), DESC));
	}

	template<std::output_iterator<const char8_t&> OutputIt>
	OutputIt vformat_to(OutputIt out, std::u8string_view fmt, format_args args) {
		auto buf = internal::iterator_buffer<OutputIt, internal::buffer_traits>(out);
		internal::vformat_to(buf, fmt, args);
		return buf.out();
	}

	inline char8_t* vformat_to(char8_t* out, std::u8string_view fmt, format_args args) {
		return internal::vformat_to(out, fmt, args);
	}

	//====================> format_to_n <=======================

	template<std::output_iterator<const char8_t&> OutputIt, typename... T>
	format_to_n_result<OutputIt> format_to_n(OutputIt out, size_t n, format_string<T...> fmt, T&&... args) {
		constexpr auto DESC = internal::make_descriptor<T...>();
		return u8lib::vformat_to_n(out, n, fmt.get(), format_args(u8lib::make_format_store(args...), DESC));
	}

	template<std::output_iterator<const char8_t&> OutputIt>
	format_to_n_result<OutputIt> vformat_to_n(OutputIt out, size_t n, std::u8string_view fmt, format_args args) {
		auto buf = internal::iterator_buffer<OutputIt, internal::fixed_buffer_traits>(out, n);
		internal::vformat_to(buf, fmt, args);
		return {buf.out(), buf.count()};
	}

	inline format_to_n_result<char8_t*> vformat_to_n(char8_t* out, size_t n, std::u8string_view fmt, format_args args) {
		return internal::vformat_to_n(out, n, fmt, args);
	}

	//===============> format_to fixed array <==================

	template<size_t N, typename... T>
	format_to_result format_to(char8_t (&out)[N], format_string<T...> fmt, T&&... args) {
		constexpr auto DESC = internal::make_descriptor<T...>();
		return u8lib::vformat_to(out, fmt.get(), format_args(u8lib::make_format_store(args...), DESC));
	}

	template<size_t N>
	format_to_result vformat_to(char8_t (&out)[N], std::u8string_view fmt, format_args args) {
		auto result = u8lib::vformat_to_n(out, N, fmt, args);
		return {result.out, result.size > N};
	}

	//===================> formatted_size <=====================

	template<typename... T>
	size_t formatted_size(format_string<T...> fmt, T&&... args) {
		constexpr auto DESC = internal::make_descriptor<T...>();
		return u8lib::vformatted_size(fmt.get(), format_args(u8lib::make_format_store(args...), DESC));
	}

	inline size_t vformatted_size(std::u8string_view fmt, format_args args) {
		return internal::vformatted_size(fmt, args);
	}
}
