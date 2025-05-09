#pragma once

#include "string.hpp"

#include <stack>
#include <vector>
#include <expected>

namespace u8lib
{
	enum class JsonErrorCode : uint8_t {
		UnknownError,      // RW
		NoOpenScope,       // RW
		ScopeTypeMismatch, // RW

		EmptyObjectFieldKey, // RW
		ArrayElementWithKey, // RW
		RootObjectWithKey,   // RW

		PresetKeyNotConsumedYet, // RW
		PresetKeyIsEmpty,        // RW

		KeyNotFound,      // R
		UnknownTypeToRead // R
	};

	class U8LIB_API JsonWriter {
	public:
		// reserve for stack
		explicit JsonWriter(size_t level_depth);
		~JsonWriter();

		std::expected<void, JsonErrorCode> start_object(u8string_view key);
		std::expected<void, JsonErrorCode> start_array(u8string_view key);
		std::expected<void, JsonErrorCode> end_array() noexcept;
		std::expected<void, JsonErrorCode> end_object() noexcept;

		std::expected<void, JsonErrorCode> write(u8string_view key, bool value);
		std::expected<void, JsonErrorCode> write(u8string_view key, int64_t value);
		std::expected<void, JsonErrorCode> write(u8string_view key, uint64_t value);
		std::expected<void, JsonErrorCode> write(u8string_view key, double value);
		std::expected<void, JsonErrorCode> write(u8string_view key, const char8_t* value, size_t len);

		std::expected<void, JsonErrorCode> write(size_t count, u8string_view key, const bool* value);
		std::expected<void, JsonErrorCode> write(size_t count, u8string_view key, const int64_t* value);
		std::expected<void, JsonErrorCode> write(size_t count, u8string_view key, const uint64_t* value);
		std::expected<void, JsonErrorCode> write(size_t count, u8string_view key, const double* value);
		std::expected<void, JsonErrorCode> write(size_t count, u8string_view key, const char8_t** value, const size_t* len);

		[[nodiscard]] u8string dump() const;

		template<typename... Args>
		std::expected<void, JsonErrorCode> write(u8string_view key, Args&&... args);

		template<typename... Args>
		std::expected<void, JsonErrorCode> write(size_t count, u8string_view key, Args&&... args);

	private:
		friend struct JsonImpl;

		struct Level {
			enum EType {
				kObject,
				kArray
			};

			struct JsonWriterValue* value;
			EType type;

			Level(JsonWriterValue* _value, EType _type) noexcept : value(_value), type(_type) {}
		};

		struct JsonWriterDocument* document;
		std::vector<Level> stack;
	};

	class U8LIB_API JsonReader {
	public:
		explicit JsonReader(const char8_t* json);
		explicit JsonReader(u8string_view json);
		explicit JsonReader(const u8string& json);
		JsonReader(const char8_t* json, size_t len);
		~JsonReader();

		std::expected<void, JsonErrorCode> start_object(u8string_view key);
		std::expected<size_t, JsonErrorCode> start_array(u8string_view key);
		std::expected<void, JsonErrorCode> end_array() noexcept;
		std::expected<void, JsonErrorCode> end_object() noexcept;

		std::expected<void, JsonErrorCode> read(u8string_view key, bool& value);
		std::expected<void, JsonErrorCode> read(u8string_view key, int64_t& value);
		std::expected<void, JsonErrorCode> read(u8string_view key, uint64_t& value);
		std::expected<void, JsonErrorCode> read(u8string_view key, double& value);
		std::expected<void, JsonErrorCode> read(u8string_view key, const char8_t*& value);

		template<typename T>
		std::expected<void, JsonErrorCode> read(u8string_view key, T& value);

		template<typename T>
		std::expected<void, JsonErrorCode> read(size_t count, T* values);

	private:
		friend struct JsonImpl;

		struct Level {
			enum EType {
				kObject,
				kArray
			};

			uint32_t index = 0;
			struct JsonReaderValue* value;
			EType type;

			Level(JsonReaderValue* _value, EType _type) noexcept : value(_value), type(_type) {}
		};

		struct JsonReaderDocument* document;
		std::stack<Level> stack;
	};
}

namespace u8lib
{
	namespace json
	{
		inline std::expected<void, JsonErrorCode> write(JsonWriter& w, u8string_view key, int8_t value) {
			return w.write(key, static_cast<int64_t>(value));
		}

		inline std::expected<void, JsonErrorCode> write(JsonWriter& w, u8string_view key, int16_t value) {
			return w.write(key, static_cast<int64_t>(value));
		}

		inline std::expected<void, JsonErrorCode> write(JsonWriter& w, u8string_view key, int32_t value) {
			return w.write(key, static_cast<int64_t>(value));
		}

		inline std::expected<void, JsonErrorCode> write(JsonWriter& w, u8string_view key, uint8_t value) {
			return w.write(key, static_cast<uint64_t>(value));
		}

		inline std::expected<void, JsonErrorCode> write(JsonWriter& w, u8string_view key, uint16_t value) {
			return w.write(key, static_cast<uint64_t>(value));
		}

		inline std::expected<void, JsonErrorCode> write(JsonWriter& w, u8string_view key, uint32_t value) {
			return w.write(key, static_cast<uint64_t>(value));
		}

		inline std::expected<void, JsonErrorCode> write(JsonWriter& w, u8string_view key, float value) {
			return w.write(key, static_cast<double>(value));
		}

		inline std::expected<void, JsonErrorCode> write(JsonWriter& w, u8string_view key, long double value) {
			return w.write(key, static_cast<double>(value));
		}

		inline std::expected<void, JsonErrorCode> write(JsonWriter& w, u8string_view key, const char8_t* value) {
			return w.write(key, value, std::char_traits<char8_t>::length(value));
		}

		inline std::expected<void, JsonErrorCode> write(JsonWriter& w, u8string_view key, u8string_view value) {
			return w.write(key, value.data(), value.size());
		}

		inline std::expected<void, JsonErrorCode> write(JsonWriter& w, u8string_view key, const u8string& value) {
			return w.write(key, value.data(), value.size());
		}

		template<typename... Args>
		concept JsonWritable = requires(JsonWriter& w, u8string_view key, Args&&... args)
		{
			{ json::write(w, key, std::forward<Args>(args)...) } -> std::same_as<std::expected<void, JsonErrorCode>>;
		};

		template<typename... Args>
		concept JsonArrayWritable = requires(JsonWriter& w, size_t count, u8string_view key, Args&&... args)
		{
			{ json::write(w, count, key, std::forward<Args>(args)...) } -> std::same_as<std::expected<void, JsonErrorCode>>;
		};
	}

	namespace json
	{
		inline std::expected<void, JsonErrorCode> read(JsonReader& r, u8string_view key, int8_t& value) {
			int64_t tmp;
			auto result = r.read(key, tmp);
			value = static_cast<int8_t>(tmp);
			return result;
		}

		inline std::expected<void, JsonErrorCode> read(JsonReader& r, u8string_view key, int16_t& value) {
			int64_t tmp;
			auto result = r.read(key, tmp);
			value = static_cast<int16_t>(tmp);
			return result;
		}

		inline std::expected<void, JsonErrorCode> read(JsonReader& r, u8string_view key, int32_t& value) {
			int64_t tmp;
			auto result = r.read(key, tmp);
			value = static_cast<int32_t>(tmp);
			return result;
		}

		inline std::expected<void, JsonErrorCode> read(JsonReader& r, u8string_view key, uint8_t& value) {
			uint64_t tmp;
			auto result = r.read(key, tmp);
			value = static_cast<uint8_t>(tmp);
			return result;
		}

		inline std::expected<void, JsonErrorCode> read(JsonReader& r, u8string_view key, uint16_t& value) {
			uint64_t tmp;
			auto result = r.read(key, tmp);
			value = static_cast<uint16_t>(tmp);
			return result;
		}

		inline std::expected<void, JsonErrorCode> read(JsonReader& r, u8string_view key, uint32_t& value) {
			uint64_t tmp;
			auto result = r.read(key, tmp);
			value = static_cast<uint32_t>(tmp);
			return result;
		}

		inline std::expected<void, JsonErrorCode> read(JsonReader& r, u8string_view key, float& value) {
			double tmp;
			auto result = r.read(key, tmp);
			value = static_cast<float>(tmp);
			return result;
		}

		inline std::expected<void, JsonErrorCode> read(JsonReader& r, u8string_view key, long double& value) {
			double tmp;
			auto result = r.read(key, tmp);
			value = static_cast<long double>(tmp);
			return result;
		}

		inline std::expected<void, JsonErrorCode> read(JsonReader& r, u8string_view key, char8_t*& value) {
			return r.read(key, const_cast<const char8_t*&>(value));
		}

		inline std::expected<void, JsonErrorCode> read(JsonReader& r, u8string_view key, u8string_view& value) {
			const char8_t* tmp;
			auto result = r.read(key, tmp);
			value = tmp;
			return result;
		}

		inline std::expected<void, JsonErrorCode> read(JsonReader& r, u8string_view key, u8string& value) {
			const char8_t* tmp;
			auto result = r.read(key, tmp);
			if (result.has_value()) value = tmp;
			return result;
		}

		template<typename T>
		concept JsonReadable = requires(JsonReader& r, u8string_view key, T& value)
		{
			{ json::read(r, key, value) } -> std::same_as<std::expected<void, JsonErrorCode>>;
		};
	}

	template<typename... Args>
	std::expected<void, JsonErrorCode> JsonWriter::write(u8string_view key, Args&&... args) {
		static_assert(json::JsonWritable<Args...>);
		return json::write(*this, key, std::forward<Args>(args)...);
	}

	template<typename... Args>
	std::expected<void, JsonErrorCode> JsonWriter::write(size_t count, u8string_view key, Args&&... args) {
		if constexpr (json::JsonArrayWritable<Args...>) {
			return json::write(*this, count, key, std::forward<Args>(args)...);
		} else {
			std::expected<void, JsonErrorCode> result = start_array(key);
			if (!result.has_value()) {
				return result;
			}

			for (size_t i = 0; i < count; i++) {
				result = this->write(u8"", std::forward<Args>(args)[i]...);
				if (!result.has_value()) {
					return result;
				}
			}
			return result;
		}
	}

	inline JsonReader::JsonReader(const char8_t* json) : JsonReader(u8string_view{json}) {}

	inline JsonReader::JsonReader(u8string_view json) : JsonReader(json.data(), json.size()) {}

	inline JsonReader::JsonReader(const u8string& json): JsonReader(json.data(), json.size()) {}

	template<typename T>
	std::expected<void, JsonErrorCode> JsonReader::read(u8string_view key, T& value) {
		static_assert(json::JsonReadable<T>);
		return json::read(*this, key, value);
	}

	template<typename T>
	std::expected<void, JsonErrorCode> JsonReader::read(size_t count, T* values) {
		using enum JsonErrorCode;

		if (stack.empty()) {
			return std::unexpected(NoOpenScope);
		}

		if (stack.top().type != Level::kArray) {
			return std::unexpected(ScopeTypeMismatch);
		}

		for (auto i = 0; i < count; i++) {
			if (auto result = this->read(u8"", values[i]); !result.has_value()) {
				return result;
			}
		}
		return {};
	}
}
