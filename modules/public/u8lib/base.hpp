#pragma once

#include "config.hpp"
#include <string_view>

//================================> internal <==================================

namespace u8lib
{
	class parse_context;
	class context;
	class format_arg;

	namespace internal
	{
		//========================> type <==========================

		enum class Type : uint8_t {
			none_type,
			int_type,
			uint_type,
			long_long_type,
			ulong_long_type,
			bool_type,
			char_type,
			last_integer_type = char_type,
			float_type,
			double_type,
			long_double_type,
			last_numeric_type = long_double_type,
			pointer_type,
			cstring_type,
			string_type,
			custom_type,
		};

		struct custom_value;

		struct format_arg_traits {
			// Function template _Type_eraser mirrors the type dispatching mechanism in the construction of basic_format_arg
			// (N4950 [format.arg]). They determine the mapping from "raw" to "erased" argument type for format_arg_store.
			static auto type_erase(bool) -> bool;
			static auto type_erase(int) -> int;
			static auto type_erase(unsigned int) -> unsigned int;
			static auto type_erase(long long) -> long long;
			static auto type_erase(unsigned long long) -> unsigned long long;
			static auto type_erase(char8_t) -> char8_t;
			static auto type_erase(float) -> float;
			static auto type_erase(double) -> double;
			static auto type_erase(long double) -> long double;
			static auto type_erase(const void*) -> const void*;
			static auto type_erase(const char8_t*) -> const char8_t*;
			static auto type_erase(std::u8string_view) -> std::u8string_view;
			static auto type_erase(auto) -> custom_value;

			template<typename T>
			using storage_type = decltype(type_erase(std::declval<std::remove_cvref_t<T>>()));
		};

		//====================> value store <=======================

		struct custom_value {
			using char_type = char8_t;
			const void* ptr_;
			void (*format_)(parse_context& parse_ctx, context& ctx, const void* arg);

			template<typename T>
			custom_value(const T& value) noexcept;

			constexpr void format(parse_context& parse_ctx, context& format_ctx) const;
		};

		struct value {
			constexpr value() noexcept;
			constexpr value(int val) noexcept;
			constexpr value(unsigned int val) noexcept;
			constexpr value(long long val) noexcept;
			constexpr value(unsigned long long val) noexcept;
			constexpr value(bool val) noexcept;
			constexpr value(char8_t val) noexcept;
			constexpr value(float val) noexcept;
			constexpr value(double val) noexcept;
			constexpr value(long double val) noexcept;
			constexpr value(const void* val) noexcept;
			constexpr value(const char8_t* val) noexcept;
			constexpr value(std::u8string_view val) noexcept;

			template<typename T>
			constexpr value(const T& val) noexcept;

			union {
				std::monostate no_state_;
				int int_state_;
				unsigned int uint_state_;
				long long long_long_state_;
				unsigned long long ulong_long_state_;
				bool bool_state_;
				char8_t char_state_;
				float float_state_;
				double double_state_;
				long double long_double_state_;
				const void* pointer_state_;
				const char8_t* cstring_state_;
				std::u8string_view string_state_;
				custom_value custom_state_;
			};
		};

		enum { packed_arg_bits            = 4 };
		enum { max_packed_args            = 63 / packed_arg_bits };
		enum : uint64_t { is_unpacked_bit = 1ULL << 63 };

		template<size_t NUM_ARGS>
		using arg_t = std::conditional_t<NUM_ARGS <= max_packed_args, value, format_arg>;

		/**
		 * An array of references to arguments. It can be implicitly converted to
		 * `format_args` for passing into type-erased formatting functions
		 * such as `vformat`. It is a plain struct to reduce binary size in debug mode.
		 */
		template<size_t NUM_ARGS>
		struct format_arg_store {
			using type = arg_t<NUM_ARGS>[NUM_ARGS];
			type args;
		};

		template<> struct format_arg_store<0> {};

		//===================> format string <======================

		template<typename...>
		class fstring {
		public:
			template<std::convertible_to<std::u8string_view> T>
			consteval fstring(const T& s);

			constexpr std::u8string_view get() const noexcept;

		private:
			std::u8string_view str;
		};

		//======================> iterator <========================

		class buffer;

		/**
		 * An output iterator that appends to a buffer. It is used instead of
		 * back_insert_iterator to reduce symbol sizes and avoid <iterator> dependency.
		 */
		struct appender {
			using value_type = void;
			using pointer = void;
			using reference = void;
			using difference_type = ptrdiff_t;
			using container_type = buffer;

			container_type* container;

			constexpr appender(buffer& buf);
			constexpr appender& operator=(char8_t c);
			constexpr appender& operator*();
			constexpr appender& operator++();
			constexpr appender operator++(int) const;
		};

		//=====================> formatter <========================

		enum class Align : uint8_t { none, left, right, center };

		enum class Sign : uint8_t { none, plus, minus, space };

		struct auto_id_tag {};

		struct basic_format_specs {
			int width_ = 0;
			int precision_ = -1;
			char8_t type_ = '\0';
			Align alignment_ = Align::none;
			Sign sgn_ = Sign::none;
			bool alt_ = false;
			bool localized_ = false;
			bool leading_zero_ = false;
			uint8_t fill_length_ = 1;
			// At most one codepoint (so one char32_t or four utf-8 char8_t).
			char8_t fill_[4 / sizeof(char8_t)] = {u8' '};
		};

		/**
		 * Adds width and precision references to basic_format_specs.
		 * This is required for formatter implementations because we must
		 * parse the format specs without having access to the format args (via a format context).
		 */
		struct dynamic_format_specs : basic_format_specs {
			int dynamic_width_index_ = -1;
			int dynamic_precision_index_ = -1;
		};

		// Model of parse_spec_callbacks that fills a basic_format_specs with the parsed data.
		class specs_setter {
		public:
			constexpr explicit specs_setter(basic_format_specs& specs);
			constexpr void on_align(Align aln) const;
			constexpr void on_fill(std::u8string_view sv) const;
			constexpr void on_sign(Sign sgn) const;
			constexpr void on_hash() const;
			constexpr void on_zero() const;
			constexpr void on_width(int width) const;
			constexpr void on_precision(int precision) const;
			constexpr void on_localized() const;
			constexpr void on_type(char8_t type) const;

		protected:
			basic_format_specs& specs_;
		};

		class dynamic_specs_handler : public specs_setter {
		public:
			constexpr dynamic_specs_handler(dynamic_format_specs& specs, parse_context& parse_ctx);
			constexpr void on_dynamic_width(size_t arg_id) const;
			constexpr void on_dynamic_width(auto_id_tag) const;
			constexpr void on_dynamic_precision(size_t arg_id) const;
			constexpr void on_dynamic_precision(auto_id_tag) const;

		private:
			dynamic_format_specs& dynamic_specs_;
			parse_context& parse_ctx_;

			static constexpr int verify_dynamic_arg_index_in_range(size_t idx);
		};

		template<typename T, Type>
		struct formatter_base {
			constexpr auto parse(parse_context& parse_ctx);
			U8LIB_API appender format(T value, context& ctx) const;

		private:
			dynamic_format_specs specs_;
		};
	}
}

//===============================> public API <=================================

namespace u8lib
{
	//===============================> struct <=================================

	class format_arg {
	public:
		using handle = internal::custom_value;

		constexpr format_arg() noexcept;
		constexpr format_arg(int val) noexcept;
		constexpr format_arg(unsigned int val) noexcept;
		constexpr format_arg(long long val) noexcept;
		constexpr format_arg(unsigned long long val) noexcept;
		constexpr format_arg(bool val) noexcept;
		constexpr format_arg(char8_t val) noexcept;
		constexpr format_arg(float val) noexcept;
		constexpr format_arg(double val) noexcept;
		constexpr format_arg(long double val) noexcept;
		constexpr format_arg(const void* val) noexcept;
		constexpr format_arg(const char8_t* val) noexcept;
		constexpr format_arg(std::u8string_view val) noexcept;
		template<typename T> constexpr format_arg(const T& val) noexcept;

		constexpr explicit operator bool() const noexcept;

		template<typename Visitor>
		constexpr decltype(auto) visit(Visitor&& vis);

		internal::value value_;
		internal::Type active_state_;
	};

	class format_args {
	public:
		constexpr format_args() noexcept;
		constexpr format_args(internal::format_arg_store<0>, size_t) noexcept;
		U8LIB_API format_args(const format_arg* args, size_t count) noexcept;

		template<size_t NUM_ARGS>
		format_args(const internal::format_arg_store<NUM_ARGS>& s, size_t desc);

		U8LIB_API format_arg get(size_t index) const noexcept;
		U8LIB_API size_t estimate_required_capacity() const noexcept;

	private:
		/**
		 * A descriptor that contains information about formatting arguments.
		 * If the number of arguments is less or equal to max_packed_args then
		 * argument types are passed in the descriptor. This reduces binary code size
		 * per formatting function call.
		 */
		uint64_t desc_;

		union {
			/**
			 * If is_packed() returns true then argument values are stored in values_;
			 * otherwise they are stored in args_. This is done to improve cache
			 * locality and reduce compiled code size since storing larger objects
			 * may require more code (at least on x86-64) even if the same amount of
			 * data is actually copied to stack. It saves ~10% on the bloat test.
			 */
			const internal::value* values_;
			const format_arg* args_;
		};

		U8LIB_API internal::Type type(size_t index) const;
		U8LIB_API bool is_packed() const;
		U8LIB_API uint64_t max_size() const;
	};

	class parse_context {
	public:
		using char_type = char8_t;
		using const_iterator = decltype(std::u8string_view().begin());
		using iterator = const_iterator;

		constexpr explicit parse_context(std::u8string_view fmt, size_t num_args = 0) noexcept;
		parse_context(const parse_context&) = delete;
		parse_context& operator=(const parse_context&) = delete;

		constexpr const_iterator begin() const noexcept;
		constexpr const_iterator end() const noexcept;
		constexpr const char8_t* unchecked_begin() const noexcept;
		constexpr const char8_t* unchecked_end() const noexcept;
		constexpr void advance_to(const const_iterator& iter);

		/**
		 * While the standard presents an exposition-only enum value for
		 * the indexing mode (manual, automatic, or unknown) we use next_arg_id_ to indicate it.
		 * next_arg_id_ > 0 means automatic
		 * next_arg_id_ == 0 means unknown
		 * next_arg_id_ < 0 means manual
		 */
		constexpr size_t next_arg_id();
		constexpr void check_arg_id(size_t id);

	private:
		std::u8string_view format_string_;
		size_t num_args_;
		/**
		 * The standard says this is size_t, however we use ptrdiff_t to save some space
		 * by not having to store the indexing mode. Above is a more detailed explanation
		 * of how this works.
		 */
		ptrdiff_t next_arg_id_ = 0;
	};

	class context {
	public:
		using iterator = internal::appender;
		using char_type = char8_t;

		context(iterator iter, format_args ctx_args, const void* loc = nullptr);

		format_arg arg(size_t id) const noexcept;
		iterator out() const;
		void advance_to(iterator it);
		const format_args& get_args() const noexcept;
		const void* get_lazy_locale() const;

	protected:
		iterator out_;
		format_args args_;
		const void* loc_;
	};

	template<typename... Args>
	using format_string = internal::fstring<std::type_identity_t<Args>...>;

	template<typename OutputIt>
	struct format_to_n_result {
		OutputIt out;
		std::iter_difference_t<OutputIt> size;
	};

	struct format_to_result {
		// Pointer to just after the last successful write in the array.
		char8_t* out;
		// Specifies if the output was truncated.
		bool truncated;

		operator char8_t*() const; // NOLINT(*-explicit-constructor)
	};

	//==============================> function <================================

	/*!
	 * @brief
	 *		Formats `args` according to specifications in `fmt`, writes the result to
	 *		the output iterator `out` and returns the iterator past the end of the output
	 *		range. `format_to` does not append a terminating null character.
	 *
	 * @code
	 *		auto out = std::vector<char8_t>();
	 *		u8lib::format_to(std::back_inserter(out), u8"{}", 42);
	 * @endcode
	 */
	template<std::output_iterator<const char8_t&> OutputIt, typename... T>
	OutputIt format_to(OutputIt out, format_string<T...> fmt, T&&... args);

	template<std::output_iterator<const char8_t&> OutputIt>
	OutputIt vformat_to(OutputIt out, std::u8string_view fmt, format_args args);

	inline char8_t* vformat_to(char8_t* out, std::u8string_view fmt, format_args args);

	//====================> format_to_n <=======================

	/*
	 * Formats `args` according to specifications in `fmt`, writes up to `n`
	 * characters of the result to the output iterator `out` and returns the total
	 * (not truncated) output size and the iterator past the end of the output
	 * range. `format_to_n` does not append a terminating null character.
	 */
	template<std::output_iterator<const char8_t&> OutputIt, typename... T>
	format_to_n_result<OutputIt> format_to_n(OutputIt out, size_t n, format_string<T...> fmt, T&&... args);

	template<std::output_iterator<const char8_t&> OutputIt>
	format_to_n_result<OutputIt> vformat_to_n(OutputIt out, size_t n, std::u8string_view fmt, format_args args);

	inline format_to_n_result<char8_t*> vformat_to_n(char8_t* out, size_t n, std::u8string_view fmt, format_args args);

	//===============> format_to fixed array <==================

	template<size_t N, typename... T>
	format_to_result format_to(char8_t (&out)[N], format_string<T...> fmt, T&&... args);

	template<size_t N>
	format_to_result vformat_to(char8_t (&out)[N], std::u8string_view fmt, format_args args);

	//===================> formatted_size <=====================

	// Returns the number of chars in the output of `format(fmt, args...)`.
	template<typename... T>
	[[nodiscard]] size_t formatted_size(format_string<T...> fmt, T&&... args);

	[[nodiscard]] inline size_t vformatted_size(std::u8string_view fmt, format_args args);

	//=============================> formatter <================================

	template<typename> struct formatter;
	template<> struct formatter<bool> : internal::formatter_base<bool, internal::Type::bool_type> {};
	template<> struct formatter<int> : internal::formatter_base<int, internal::Type::int_type> {};
	template<> struct formatter<unsigned int> : internal::formatter_base<unsigned int, internal::Type::uint_type> {};
	template<> struct formatter<long long> : internal::formatter_base<long long, internal::Type::long_long_type> {};
	template<> struct formatter<unsigned long long> : internal::formatter_base<unsigned long long, internal::Type::ulong_long_type> {};
	template<> struct formatter<char8_t> : internal::formatter_base<char8_t, internal::Type::char_type> {};
	template<> struct formatter<float> : internal::formatter_base<float, internal::Type::float_type> {};
	template<> struct formatter<double> : internal::formatter_base<double, internal::Type::double_type> {};
	template<> struct formatter<long double> : internal::formatter_base<long double, internal::Type::long_double_type> {};
	template<> struct formatter<const void*> : internal::formatter_base<const void*, internal::Type::pointer_type> {};
	template<> struct formatter<const char8_t*> : internal::formatter_base<const char8_t*, internal::Type::cstring_type> {};
	template<> struct formatter<std::u8string_view> : internal::formatter_base<std::u8string_view, internal::Type::string_type> {};

	template<> struct formatter<signed char> : formatter<int> {};
	template<> struct formatter<short> : formatter<int> {};
	template<> struct formatter<long> : formatter<std::conditional_t<sizeof(long) == sizeof(int), int, long long>> {};
	template<> struct formatter<unsigned char> : formatter<unsigned int> {};
	template<> struct formatter<unsigned short> : formatter<unsigned int> {};
	template<> struct formatter<unsigned long> : formatter<std::conditional_t<sizeof(unsigned long) == sizeof(unsigned int), unsigned int, unsigned long long>> {};
	template<> struct formatter<char> : formatter<char8_t> {};
	template<> struct formatter<void*> : formatter<const void*> {};
	template<> struct formatter<char8_t*> : formatter<const char8_t*> {};

	template<size_t N>
	struct formatter<char8_t[N]> : formatter<std::u8string_view> {
		using base = formatter<std::u8string_view>;

		context::iterator format(const char8_t (&value)[N], context& ctx) const {
			return base::format(std::u8string_view{value, N - 1}, ctx);
		}
	};

	template<>
	struct formatter<const char*> : formatter<const char8_t*> {
		using base = formatter<const char8_t*>;

		context::iterator format(const char* value, context& ctx) const {
			return base::format(reinterpret_cast<const char8_t*>(value), ctx);
		}
	};

	template<> struct formatter<char*> : formatter<const char*> {};

	template<>
	struct formatter<std::string_view> : formatter<std::u8string_view> {
		using base = formatter<std::u8string_view>;

		context::iterator format(const std::string_view value, context& ctx) const {
			return base::format(std::u8string_view{reinterpret_cast<const char8_t*>(value.data()), value.size()}, ctx);
		}
	};

	template<size_t N>
	struct formatter<char[N]> : formatter<std::string_view> {
		using base = formatter<std::string_view>;

		context::iterator format(const char (&value)[N], context& ctx) const {
			return base::format(std::string_view{value, N - 1}, ctx);
		}
	};

	template<>
	struct formatter<std::nullptr_t> : formatter<std::u8string_view> {
		using base = formatter<std::u8string_view>;

		context::iterator format(std::nullptr_t, context& ctx) const {
			return base::format(std::u8string_view{u8"nullptr"}, ctx);
		}
	};

	template<typename T, typename U = std::remove_const_t<T>>
	concept formattable = requires(formatter<U>& f, const formatter<U>& cf, T&& t, context fc, parse_context pc)
	{
		{ f.parse(pc) } -> std::same_as<parse_context::iterator>;
		{ cf.format(t, fc) } -> std::same_as<context::iterator>;
	} && std::semiregular<formatter<U>>;
}

#include "implement/base.inl"
