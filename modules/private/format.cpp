#include "pch.hpp"

#include <u8lib/format.hpp>

#include <bit>
#include <stdexcept>

namespace u8lib::internal
{
	template<typename T> requires(std::is_arithmetic_v<T> && !is_any_of_v<T, char8_t, bool>)
	appender write(appender out, T value);

	template<std::integral T> requires(!is_any_of_v<T, char8_t, bool>)
	appender write(appender out, T value, const basic_format_specs& specs, const std::locale* loc);

	template<std::floating_point T>
	appender write(appender out, T value, const basic_format_specs& specs, const std::locale* loc);

	appender write(appender, std::monostate);
	appender write(appender out, bool value);
	appender write(appender out, char8_t value);
	appender write(appender out, const void* value);
	appender write(appender out, const char8_t* value);
	appender write(appender out, std::u8string_view value);
	appender write(appender, std::monostate, const basic_format_specs&, const std::locale*);
	appender write(appender out, bool value, basic_format_specs specs, const std::locale* loc);
	appender write(appender out, char8_t value, basic_format_specs specs, const std::locale* loc);
	appender write(appender out, const void* value, const basic_format_specs& specs, const std::locale*);
	appender write(appender out, const char8_t* value, const basic_format_specs& specs, const std::locale* loc);
	appender write(appender out, std::u8string_view value, const basic_format_specs& specs, const std::locale*);
}

namespace u8lib::internal
{
	// Fetch the value of an argument associated with a dynamic
	// width or precision specifier. This will be called with either
	// width_checker or precision_checker as "Handler".
	template<typename Handler>
	int get_dynamic_specs(format_arg arg) {
		const unsigned long long value = arg.visit(Handler{});
		if (value > static_cast<unsigned long long>(std::numeric_limits<int>::max())) {
			report_error(u8"number is too big.");
		}

		return static_cast<int>(value);
	}

	template<typename T>
	concept is_signed_or_unsigned_large_integer_t = std::integral<T> && sizeof(T) >= sizeof(int);

	// Checks that the type and value of an argument associated with a dynamic
	// width specifier are valid.
	class width_checker {
	public:
		template<typename T>
		constexpr unsigned long long operator()(const T value) const {
			if constexpr (is_signed_or_unsigned_large_integer_t<T>) {
				if constexpr (std::is_signed_v<T>) {
					if (value < 0) {
						report_error(u8"negative width.");
					}
				}
				return static_cast<unsigned long long>(value);
			} else {
				report_error(u8"width is not an integer.");
				std::unreachable();
			}
		}
	};

	// Checks that the type and value of an argument associated with a dynamic
	// precision specifier are valid.
	class precision_checker {
	public:
		template<typename T>
		constexpr unsigned long long operator()(const T value) const {
			if constexpr (is_signed_or_unsigned_large_integer_t<T>) {
				if constexpr (std::is_signed_v<T>) {
					if (value < 0) {
						report_error(u8"negative precision.");
					}
				}
				return static_cast<unsigned long long>(value);
			} else {
				report_error(u8"precision is not an integer.");
				std::unreachable();
			}
		}
	};

	format_arg get_arg(const context& ctx, const size_t arg_id) {
		// note: while this is parameterized on the _Arg_id type in libfmt we don't
		// need to do that in std::format because it's only called with either an integer
		// id or a named id (which we do not support in std::format)
		auto&& arg = ctx.arg(arg_id);
		if (!arg) {
			report_error(u8"argument not found.");
		}

		return arg;
	}

	// Parses standard format specs into a basic_format_specs using specs_setter, and
	// additionally handles dynamic width and precision. This is separate from specs_setter
	// because it needs to know about the current parse_context and context
	// in order to fetch the width from the arguments.
	class specs_handler : public specs_setter {
	public:
		constexpr specs_handler(basic_format_specs& specs, parse_context& parse_ctx, context& ctx)
			: specs_setter(specs), parse_ctx_(parse_ctx), ctx_(ctx) {}

		template<typename ID>
		constexpr void on_dynamic_width(const ID arg_id) {
			this->specs_.width_ = get_dynamic_specs<width_checker>(this->get_arg(arg_id));
		}

		template<typename ID>
		constexpr void on_dynamic_precision(const ID arg_id) {
			this->specs_.precision_ = get_dynamic_specs<precision_checker>(this->get_arg(arg_id));
		}

	private:
		parse_context& parse_ctx_;
		context& ctx_;

		format_arg get_arg(auto_id_tag) const {
			return internal::get_arg(ctx_, parse_ctx_.next_arg_id());
		}

		format_arg get_arg(const size_t arg_id) const {
			parse_ctx_.check_arg_id(arg_id);
			return internal::get_arg(ctx_, arg_id);
		}
	};

	// This is the visitor that's used for "simple" replacement fields.
	// It could be a generic lambda, but that's bad for throughput.
	// A simple replacement field is a replacement field that's just "{}",
	// without any format specs.
	struct default_arg_formatter {
		context::iterator out_;
		format_args args_;
		const void* loc_;

		template<typename T>
		context::iterator operator()(T value) && {
			return internal::write(out_, value);
		}

		context::iterator operator()(format_arg::handle handle) && {
			parse_context parse_ctx({});
			context format_ctx(out_, args_, loc_);
			handle.format(parse_ctx, format_ctx);
			return format_ctx.out();
		}
	};

	// Visitor used for replacement fields that contain specs
	struct arg_formatter {
		context* ctx_ = nullptr;
		basic_format_specs* specs_ = nullptr;

		template<typename T>
		context::iterator operator()(T val) {
			assert(ctx_);
			assert(specs_);
			return internal::write(ctx_->out(), val, *specs_, static_cast<const std::locale*>(ctx_->get_lazy_locale()));
		}

		context::iterator operator()(format_arg::handle) {
			std::unreachable();
		}
	};

	struct format_handler {
		parse_context parse_context_;
		context ctx_;

		explicit format_handler(context::iterator out, std::u8string_view str, format_args format_args)
			: parse_context_(str), ctx_(out, format_args) {}

		explicit format_handler(context::iterator out, std::u8string_view str, format_args format_args, const void* loc)
			: parse_context_(str), ctx_(out, format_args, loc) {}

		void on_text(const char8_t* first, const char8_t* last) {
			ctx_.advance_to(std::ranges::copy(first, last, ctx_.out()).out);
		}

		void on_replacement_field(const size_t id, const char8_t*) {
			auto&& arg = get_arg(ctx_, id);
			ctx_.advance_to(arg.visit(default_arg_formatter{ctx_.out(), ctx_.get_args(), ctx_.get_lazy_locale()}));
		}

		const char8_t* on_format_specs(const size_t id, const char8_t* first, const char8_t* last) {
			parse_context_.advance_to(parse_context_.begin() + (first - std::to_address(parse_context_.begin())));
			auto&& arg = get_arg(ctx_, id);
			if (arg.active_state_ == Type::custom_type) {
				arg.value_.custom_state_.format(parse_context_, ctx_);
				return std::to_address(parse_context_.begin());
			}

			basic_format_specs specs;
			specs_checker handler(specs_handler{specs, parse_context_, ctx_}, arg.active_state_);
			first = parse_format_specs(first, last, handler);
			if (first == last || *first != '}') {
				report_error(u8"missing '}' in format string.");
			}

			ctx_.advance_to(arg.visit(arg_formatter{std::addressof(ctx_), std::addressof(specs)}));
			return first;
		}
	};
}

//===============================> implement <==================================

namespace u8lib
{
	format_args::format_args(const format_arg* args, size_t count) noexcept
		: desc_(internal::is_unpacked_bit | count), args_(args) {}

	format_arg format_args::get(size_t index) const noexcept {
		auto arg = format_arg();
		if (!is_packed()) {
			if (index < max_size()) arg = args_[index];
			return arg;
		}
		if (static_cast<unsigned>(index) >= internal::max_packed_args) return arg;
		arg.active_state_ = type(index);
		if (arg.active_state_ != internal::Type::none_type) arg.value_ = values_[index];
		return arg;
	}

	size_t format_args::estimate_required_capacity() const noexcept {
		size_t result = 0;

		for (auto i = 0; i < max_size(); ++i) {
			switch (type(i)) {
				case internal::Type::string_type:
					result += get(i).value_.string_state_.size();
					break;
				case internal::Type::cstring_type:
					result += 32;
					break;
				case internal::Type::none_type:
					return result;
				default:
					result += 8;
					break;
			}
		}
		return result;
	}

	internal::Type format_args::type(size_t index) const {
		const size_t shift = index * internal::packed_arg_bits;
		constexpr unsigned mask = (1 << internal::packed_arg_bits) - 1;
		return static_cast<internal::Type>((desc_ >> shift) & mask);
	}

	bool format_args::is_packed() const {
		return (desc_ & internal::is_unpacked_bit) == 0;
	}

	uint64_t format_args::max_size() const {
		return is_packed() ? internal::max_packed_args : desc_ & ~internal::is_unpacked_bit;
	}

	namespace internal
	{
		void report_error(const char8_t* message) {
			throw std::runtime_error(reinterpret_cast<const char*>(message));
		}

		template<typename T, Type ArgType>
		appender formatter_base<T, ArgType>::format(T value, context& ctx) const {
			dynamic_format_specs format_specs = specs_;
			if (specs_.dynamic_width_index_ >= 0) {
				format_specs.width_ = get_dynamic_specs<width_checker>(ctx.arg(static_cast<size_t>(specs_.dynamic_width_index_)));
			}

			if (specs_.dynamic_precision_index_ >= 0) {
				format_specs.precision_ = get_dynamic_specs<precision_checker>(ctx.arg(static_cast<size_t>(specs_.dynamic_precision_index_)));
			}

			using erased_type = format_arg_traits::storage_type<decltype(value)>;
			return format_arg(static_cast<erased_type>(value)).visit(arg_formatter{std::addressof(ctx), std::addressof(format_specs)});
		}

		u8string vformat(std::u8string_view fmt, format_args args, const void* loc) {
			u8string out;
			out.reserve(args.estimate_required_capacity());
			vformat_to(std::back_inserter(out), *static_cast<const std::locale*>(loc), fmt, args);
			return out;
		}

		void vformat_to(buffer& buf, std::u8string_view fmt, format_args args, const void* loc) {
			auto out = appender{buf};
			format_handler handler(out, fmt, args, loc);
			parse_format_string(fmt, handler);
		}

		std::remove_cvref_t<char8_t*> vformat_to(char8_t* out, std::u8string_view fmt, format_args args, const void* loc) {
			struct char_buffer : buffer {
				explicit char_buffer(char8_t* out) : buffer([](buffer*, size_t) {}, out, 0, ~size_t()) {}
			} buf{out};

			vformat_to(buf, fmt, args, loc);
			return out;
		}

		format_to_n_result<char8_t*> vformat_to_n(char8_t* out, size_t n, std::u8string_view fmt, format_args args, const void* loc) {
			struct char_buffer : fixed_buffer_traits, buffer {
				enum { buffer_size = 256 };
				char8_t data_[buffer_size] = {};
				char8_t* out_;

				static void grow(buffer* buf, size_t) {
					if (buf->size() == buf->capacity())
						reinterpret_cast<char_buffer*>(buf)->flush();
				}

				void flush() {
					size_t n = this->limit(this->size());
					if (this->data() == out_) {
						out_ += n;
						this->set(data_, buffer_size);
					}
					this->clear();
				}

				char_buffer(char8_t* out, size_t n): fixed_buffer_traits(n), buffer(grow, out, 0, n), out_(out) {}

				char_buffer(char_buffer&& other) noexcept
					: fixed_buffer_traits(other),
					buffer(static_cast<char_buffer&&>(other)),
					out_(other.out_) {
					if (this->data() != out_) {
						this->set(data_, buffer_size);
						this->clear();
					}
				}

				~char_buffer() { flush(); }

				char8_t* out() {
					flush();
					return out_;
				}

				size_t count() const {
					return fixed_buffer_traits::count() + this->size();
				}
			} buf = {out, n};

			vformat_to(buf, fmt, args, loc);
			return {buf.out(), static_cast<std::iter_difference_t<char8_t*>>(buf.count())};
		}

		size_t vformatted_size(std::u8string_view fmt, format_args args, const void* loc) {
			// A buffer that counts the number of code units written discarding the output.
			struct counting_buffer : buffer {
				enum { buffer_size = 256 };
				char8_t data_[buffer_size] = {};
				size_t count_ = 0;

				static void grow(buffer* buf, size_t) {
					if (buf->size() != buffer_size) return;
					reinterpret_cast<counting_buffer*>(buf)->count_ += buf->size();
					buf->clear();
				}

				counting_buffer() : buffer(grow, data_, 0, buffer_size) {}

				size_t count() const noexcept {
					return count_ + this->size();
				}
			} buf;

			vformat_to(buf, fmt, args, loc);
			return buf.count();
		}

		template U8LIB_API context::iterator formatter_base<bool, Type::bool_type>::format(bool value, context& ctx) const;
		template U8LIB_API context::iterator formatter_base<int, Type::int_type>::format(int value, context& ctx) const;
		template U8LIB_API context::iterator formatter_base<unsigned int, Type::uint_type>::format(unsigned int value, context& ctx) const;
		template U8LIB_API context::iterator formatter_base<long long, Type::long_long_type>::format(long long value, context& ctx) const;
		template U8LIB_API context::iterator formatter_base<unsigned long long, Type::ulong_long_type>::format(unsigned long long value, context& ctx) const;
		template U8LIB_API context::iterator formatter_base<char8_t, Type::char_type>::format(char8_t value, context& ctx) const;
		template U8LIB_API context::iterator formatter_base<float, Type::float_type>::format(float value, context& ctx) const;
		template U8LIB_API context::iterator formatter_base<double, Type::double_type>::format(double value, context& ctx) const;
		template U8LIB_API context::iterator formatter_base<long double, Type::long_double_type>::format(long double value, context& ctx) const;
		template U8LIB_API context::iterator formatter_base<const void*, Type::pointer_type>::format(const void* value, context& ctx) const;
		template U8LIB_API context::iterator formatter_base<const char8_t*, Type::cstring_type>::format(const char8_t* value, context& ctx) const;
		template U8LIB_API context::iterator formatter_base<std::u8string_view, Type::string_type>::format(std::u8string_view value, context& ctx) const;
	}
}

#include "format/write.hpp"
