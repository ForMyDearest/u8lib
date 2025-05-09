#include "pch.hpp"

#include <u8lib/json.hpp>
#include <u8lib/format.hpp>

#include <yyjson.h>

namespace u8lib
{
	struct JsonWriterDocument : yyjson_mut_doc {};

	struct JsonWriterValue : yyjson_mut_val {};

	struct JsonReaderDocument : yyjson_doc {};

	struct JsonReaderValue : yyjson_val {};

	const char* CStringBuild(yyjson_mut_doc* doc, u8string_view sv) {
		return yyjson_mut_get_str(yyjson_mut_strncpy(doc, reinterpret_cast<const char*>(sv.data()), sv.size()));
	}

	struct JsonImpl {
		template<typename T, typename... Args>
		static std::expected<void, JsonErrorCode> write(const JsonWriter& w, u8string_view key, T value, Args&&... args) {
			using enum JsonErrorCode;
			using enum JsonWriter::Level::EType;

			if (w.stack.empty()) {
				return std::unexpected(NoOpenScope);
			}

			const auto level = w.stack.back();
			bool success = false;

			if (level.type == kObject) {
				if (key.empty()) {
					return std::unexpected(EmptyObjectFieldKey);
				}
				const char* ckey = CStringBuild(w.document, key);

				if constexpr (std::is_same_v<T, bool>) {
					success = yyjson_mut_obj_add_bool(w.document, level.value, ckey, value);
				} else if constexpr (std::is_same_v<T, int64_t>) {
					success = yyjson_mut_obj_add_sint(w.document, level.value, ckey, value);
				} else if constexpr (std::is_same_v<T, uint64_t>) {
					success = yyjson_mut_obj_add_uint(w.document, level.value, ckey, value);
				} else if constexpr (std::is_floating_point_v<T>) {
					success = yyjson_mut_obj_add_real(w.document, level.value, ckey, value);
				} else {
					success = yyjson_mut_obj_add_strncpy(w.document, level.value, ckey, value, std::forward<Args>(args)...);
				}
			} else if (level.type == kArray) {
				if (!key.empty()) {
					return std::unexpected(ArrayElementWithKey);
				}

				if constexpr (std::is_same_v<T, bool>) {
					success = yyjson_mut_arr_add_bool(w.document, level.value, value);
				} else if constexpr (std::is_same_v<T, int64_t>) {
					success = yyjson_mut_arr_add_sint(w.document, level.value, value);
				} else if constexpr (std::is_same_v<T, uint64_t>) {
					success = yyjson_mut_arr_add_uint(w.document, level.value, value);
				} else if constexpr (std::is_floating_point_v<T>) {
					success = yyjson_mut_arr_add_real(w.document, level.value, value);
				} else {
					success = yyjson_mut_arr_add_strncpy(w.document, level.value, value, std::forward<Args>(args)...);
				}
			}
			if (success) {
				return {};
			}
			return std::unexpected(UnknownError);
		}

		template<typename T, typename... Args>
		static std::expected<void, JsonErrorCode> write_array(JsonWriter& w, size_t count, u8string_view key, T&& values, Args&&... args) {
			using enum JsonErrorCode;
			using enum JsonWriter::Level::EType;

			using U = std::decay_t<T>;

			if (w.stack.empty()) {
				return std::unexpected(NoOpenScope);
			}
			yyjson_mut_val* arr = nullptr;

			if constexpr (std::is_same_v<U, const bool*>) {
				arr = ::yyjson_mut_arr_with_bool(w.document, values, count);
			} else if constexpr (std::is_same_v<U, const int64_t*>) {
				arr = ::yyjson_mut_arr_with_sint64(w.document, values, count);
			} else if constexpr (std::is_same_v<U, const uint64_t*>) {
				arr = ::yyjson_mut_arr_with_uint64(w.document, values, count);
			} else if constexpr (std::is_same_v<U, const double*>) {
				arr = ::yyjson_mut_arr_with_real(w.document, values, count);
			} else {
				arr = ::yyjson_mut_arr_with_strncpy(w.document, values, std::forward<Args>(args)..., count);
			}

			const auto level = w.stack.back();
			bool success = false;

			if (level.type == kArray) {
				if (!key.empty()) {
					return std::unexpected(ArrayElementWithKey);
				}

				success = yyjson_mut_arr_add_val(level.value, arr);
			} else if (level.type == kObject) {
				if (key.empty()) {
					return std::unexpected(EmptyObjectFieldKey);
				}
				const char* ckey = CStringBuild(w.document, key);
				success = yyjson_mut_obj_add_val(w.document, level.value, ckey, arr);
			}

			if (success) {
				w.stack.emplace_back(reinterpret_cast<JsonWriterValue*>(arr), kArray);
				return {};
			}
			return std::unexpected(UnknownError);
		}

		template<typename T>
		static std::expected<void, JsonErrorCode> read(JsonReader& r, u8string_view key, T& value) {
			using enum JsonErrorCode;
			using enum JsonReader::Level::EType;

			if (r.stack.empty()) {
				return std::unexpected(NoOpenScope);
			}

			auto type = r.stack.top().type;
			auto parent = r.stack.top().value;
			const char* ckey = reinterpret_cast<const char*>(key.data());
			yyjson_val* found = nullptr;

			if (type == kObject) {
				if (key.empty()) {
					return std::unexpected(EmptyObjectFieldKey);
				}

				found = yyjson_obj_get(parent, ckey);
				if (!found) {
					return std::unexpected(KeyNotFound);
				}
			} else if (type == kArray) {
				if (!key.empty()) {
					return std::unexpected(ArrayElementWithKey);
				}

				found = yyjson_arr_get(parent, r.stack.top().index++);
				if (!found) {
					return std::unexpected(KeyNotFound);
				}
			}

			if constexpr (std::is_same_v<T, bool>) {
				value = yyjson_get_bool(found);
			} else if constexpr (std::is_same_v<T, int64_t>) {
				value = yyjson_get_sint(found);
			} else if constexpr (std::is_same_v<T, uint64_t>) {
				value = yyjson_get_uint(found);
			} else if constexpr (std::is_same_v<T, double>) {
				value = yyjson_get_real(found);
			} else {
				value = yyjson_get_str(found);
			}
			return {};
		}
	};
}

namespace u8lib
{
	JsonWriter::JsonWriter(size_t level_depth) {
		document = reinterpret_cast<JsonWriterDocument*>(yyjson_mut_doc_new(nullptr));
		stack.reserve(level_depth);
	}

	JsonWriter::~JsonWriter() {
		yyjson_mut_doc_free(document);
	}

	std::expected<void, JsonErrorCode> JsonWriter::start_object(u8string_view key) {
		using enum JsonErrorCode;

		yyjson_mut_val* obj = yyjson_mut_obj(document);

		if (stack.empty()) {
			if (!key.empty()) {
				return std::unexpected(RootObjectWithKey);
			}

			yyjson_mut_doc_set_root(document, obj);
			stack.emplace_back(reinterpret_cast<JsonWriterValue*>(obj), Level::kObject);
			return {};
		}

		const auto level = stack.back();

		if (level.type == Level::kArray) {
			if (!key.empty()) {
				return std::unexpected(ArrayElementWithKey);
			}

			bool success = yyjson_mut_arr_add_val(level.value, obj);
			if (success) {
				stack.emplace_back(reinterpret_cast<JsonWriterValue*>(obj), Level::kObject);
				return {};
			}
		} else if (level.type == Level::kObject) {
			if (key.empty()) {
				return std::unexpected(EmptyObjectFieldKey);
			}

			const char* ckey = CStringBuild(document, key);
			bool success = yyjson_mut_obj_add_val(document, level.value, ckey, obj);
			if (success) {
				stack.emplace_back(reinterpret_cast<JsonWriterValue*>(obj), Level::kObject);
				return {};
			}
		}

		return std::unexpected(UnknownError);
	}

	std::expected<void, JsonErrorCode> JsonWriter::start_array(u8string_view key) {
		using enum JsonErrorCode;

		if (stack.empty()) {
			return std::unexpected(NoOpenScope);
		}

		yyjson_mut_val* arr = yyjson_mut_arr(document);
		auto [array,type] = stack.back();
		bool success = false;

		if (type == Level::kArray) {
			if (!key.empty()) {
				return std::unexpected(ArrayElementWithKey);
			}
			success = yyjson_mut_arr_add_val(array, arr);
		} else if (type == Level::kObject) {
			if (key.empty()) {
				return std::unexpected(EmptyObjectFieldKey);
			}
			const char* ckey = CStringBuild(document, key);
			success = yyjson_mut_obj_add_val(document, array, ckey, arr);
		}

		stack.emplace_back(reinterpret_cast<JsonWriterValue*>(arr), Level::kArray);
		if (success) return {};
		return std::unexpected(UnknownError);
	}

	std::expected<void, JsonErrorCode> JsonWriter::end_array() noexcept {
		using enum JsonErrorCode;
		if (stack.empty()) {
			return std::unexpected(NoOpenScope);
		}
		if (stack.back().type != Level::kArray) {
			return std::unexpected(ScopeTypeMismatch);
		}
		stack.pop_back();
		return {};
	}

	std::expected<void, JsonErrorCode> JsonWriter::end_object() noexcept {
		using enum JsonErrorCode;
		if (stack.empty()) {
			return std::unexpected(NoOpenScope);
		}
		if (stack.back().type != Level::kObject) {
			return std::unexpected(ScopeTypeMismatch);
		}
		stack.pop_back();
		return {};
	}

	u8string JsonWriter::dump() const {
		auto str = yyjson_mut_write(document, 0, nullptr);
		u8string ret = str;
		free(str);
		return ret;
	}

	std::expected<void, JsonErrorCode>
	JsonWriter::write(u8string_view key, bool value) {
		return JsonImpl::write(*this, key, value);
	}

	std::expected<void, JsonErrorCode>
	JsonWriter::write(u8string_view key, int64_t value) {
		return JsonImpl::write(*this, key, value);
	}

	std::expected<void, JsonErrorCode>
	JsonWriter::write(u8string_view key, uint64_t value) {
		return JsonImpl::write(*this, key, value);
	}

	std::expected<void, JsonErrorCode>
	JsonWriter::write(u8string_view key, double value) {
		return JsonImpl::write(*this, key, value);
	}

	std::expected<void, JsonErrorCode>
	JsonWriter::write(u8string_view key, const char8_t* value, size_t len) {
		return JsonImpl::write(*this, key, reinterpret_cast<const char*>(value), len);
	}

	std::expected<void, JsonErrorCode> JsonWriter::write(size_t count, u8string_view key, const bool* value) {
		return JsonImpl::write_array(*this, count, key, value);
	}

	std::expected<void, JsonErrorCode> JsonWriter::write(size_t count, u8string_view key, const int64_t* value) {
		return JsonImpl::write_array(*this, count, key, value);
	}

	std::expected<void, JsonErrorCode> JsonWriter::write(size_t count, u8string_view key, const uint64_t* value) {
		return JsonImpl::write_array(*this, count, key, value);
	}

	std::expected<void, JsonErrorCode> JsonWriter::write(size_t count, u8string_view key, const double* value) {
		return JsonImpl::write_array(*this, count, key, value);
	}

	std::expected<void, JsonErrorCode> JsonWriter::write(size_t count, u8string_view key, const char8_t** value, const size_t* len) {
		{
			return JsonImpl::write_array(*this, count, key, reinterpret_cast<const char**>(value), len);
		}
	}
}

namespace u8lib
{
	JsonReader::JsonReader(const char8_t* json, size_t len) {
		yyjson_read_err err = {};
		document = reinterpret_cast<JsonReaderDocument*>(yyjson_read_opts(reinterpret_cast<char*>(const_cast<char8_t*>(json)), len, 0, nullptr, &err));
		if (document == nullptr) {
			auto s = format(u8"Failed to parse JSON: {}, error: {}", u8string_view{json, len}, err.msg);
			internal::report_error(s.data());
		}
	}

	JsonReader::~JsonReader() {
		yyjson_doc_free(document);
	}

	std::expected<void, JsonErrorCode> JsonReader::start_object(u8string_view key) {
		using enum JsonErrorCode;

		if (stack.empty()) {
			if (!key.empty()) {
				return std::unexpected(RootObjectWithKey);
			}

			auto obj = yyjson_doc_get_root(document);
			if (yyjson_get_type(obj) != YYJSON_TYPE_OBJ) {
				return std::unexpected(ScopeTypeMismatch);
			}

			stack.emplace(reinterpret_cast<JsonReaderValue*>(obj), Level::kObject);
			return {};
		}

		auto parent = stack.top().value;
		auto parent_type = yyjson_get_type(parent);

		if (parent_type == YYJSON_TYPE_ARR) {
			if (!key.empty()) {
				return std::unexpected(ArrayElementWithKey);
			}

			auto obj = yyjson_arr_get(parent, stack.top().index++);
			if (!obj) {
				return std::unexpected(KeyNotFound);
			}

			if (yyjson_get_type(obj) != YYJSON_TYPE_OBJ) {
				return std::unexpected(ScopeTypeMismatch);
			}

			stack.emplace(reinterpret_cast<JsonReaderValue*>(obj), Level::kObject);
			return {};
		}

		if (parent_type == YYJSON_TYPE_OBJ) {
			if (key.empty()) {
				return std::unexpected(EmptyObjectFieldKey);
			}

			auto obj = yyjson_obj_get(parent, reinterpret_cast<const char*>(key.data()));
			if (!obj) {
				return std::unexpected(KeyNotFound);
			}

			if (yyjson_get_type(obj) != YYJSON_TYPE_OBJ) {
				return std::unexpected(ScopeTypeMismatch);
			}

			stack.emplace(reinterpret_cast<JsonReaderValue*>(obj), Level::kObject);
			return {};
		}

		return std::unexpected(UnknownError);
	}

	std::expected<size_t, JsonErrorCode> JsonReader::start_array(u8string_view key) {
		using enum JsonErrorCode;

		if (stack.empty()) {
			return std::unexpected(NoOpenScope);
		}

		auto parent = stack.top().value;
		auto parent_type = yyjson_get_type(parent);

		if (parent_type == YYJSON_TYPE_ARR) {
			if (!key.empty()) {
				return std::unexpected(ArrayElementWithKey);
			}

			auto arr = yyjson_arr_get(parent, stack.top().index++);
			if (!arr) {
				return std::unexpected(KeyNotFound);
			}

			if (yyjson_get_type(arr) != YYJSON_TYPE_ARR) {
				return std::unexpected(ScopeTypeMismatch);
			}

			stack.emplace(reinterpret_cast<JsonReaderValue*>(arr), Level::kArray);
			return yyjson_arr_size(arr);
		}

		if (parent_type == YYJSON_TYPE_OBJ) {
			if (key.empty()) {
				return std::unexpected(EmptyObjectFieldKey);
			}
			auto arr = yyjson_obj_get(parent, reinterpret_cast<const char*>(key.data()));
			if (!arr) {
				return std::unexpected(KeyNotFound);
			}

			if (yyjson_get_type(arr) != YYJSON_TYPE_ARR) {
				return std::unexpected(ScopeTypeMismatch);
			}

			stack.emplace(reinterpret_cast<JsonReaderValue*>(arr), Level::kArray);
			return yyjson_arr_size(arr);
		}

		return std::unexpected(UnknownError);
	}

	std::expected<void, JsonErrorCode> JsonReader::end_array() noexcept {
		using enum JsonErrorCode;
		if (stack.empty()) {
			return std::unexpected(NoOpenScope);
		}
		if (stack.top().type != Level::kArray) {
			return std::unexpected(ScopeTypeMismatch);
		}
		stack.pop();
		return {};
	}

	std::expected<void, JsonErrorCode> JsonReader::end_object() noexcept {
		using enum JsonErrorCode;
		if (stack.empty()) {
			return std::unexpected(NoOpenScope);
		}
		if (stack.top().type != Level::kObject) {
			return std::unexpected(ScopeTypeMismatch);
		}
		stack.pop();
		return {};
	}

	std::expected<void, JsonErrorCode> JsonReader::read(u8string_view key, bool& value) {
		return JsonImpl::read(*this, key, value);
	}

	std::expected<void, JsonErrorCode> JsonReader::read(u8string_view key, int64_t& value) {
		return JsonImpl::read(*this, key, value);
	}

	std::expected<void, JsonErrorCode> JsonReader::read(u8string_view key, uint64_t& value) {
		return JsonImpl::read(*this, key, value);
	}

	std::expected<void, JsonErrorCode> JsonReader::read(u8string_view key, double& value) {
		return JsonImpl::read(*this, key, value);
	}

	std::expected<void, JsonErrorCode> JsonReader::read(u8string_view key, const char8_t*& value) {
		return JsonImpl::read(*this, key, reinterpret_cast<const char*&>(value));
	}
}
