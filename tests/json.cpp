#include <doctest/doctest.h>

#include <u8lib/log.hpp>
#include <u8lib/json.hpp>

#define U8LIB_STRINGIZING(...)			#__VA_ARGS__
#define U8LIB_MAKE_STRING(...)			U8LIB_STRINGIZING(__VA_ARGS__)
#define U8LIB_FILE_LINE					__FILE__ ":" U8LIB_MAKE_STRING(__LINE__)

#define U8LIB_LOG(level, format, ...)	\
	do {	\
		if (!u8lib::log::check_log_level(u8lib::log::LogLevel::level)) break;	\
		u8lib::log::log(reinterpret_cast<const char8_t*>(U8LIB_FILE_LINE), reinterpret_cast<const char8_t*>(__FUNCTION__), u8lib::log::LogLevel::level, format, ##__VA_ARGS__);	\
	} while (0);

#define LOG_INFO(format, ...)	U8LIB_LOG(info, format, ##__VA_ARGS__)

struct JSONTests {
	JSONTests() {
		u8lib::log::set_log_file(u8"json.log", true);
	}
};

template<typename T, typename U>
void CHECK_VALUE(const T& x, const U& y) {
	using namespace u8lib;
	if constexpr (std::is_same_v<T, char8_t*> || std::is_same_v<T, const char8_t*> || std::is_same_v<T, u8string> || std::is_same_v<T, u8string_view>) {
		CHECK_EQ(u8string(x), u8string(y));
	} else if constexpr (std::is_floating_point_v<T>) {
		CHECK(std::abs(x- y) < 0.01);
	} else {
		CHECK_EQ(x, y);
	}
}

template<typename T>
void CHECK_OK(const std::expected<T, u8lib::JsonErrorCode>& r) {
	using enum u8lib::JsonErrorCode;
	CHECK(r.has_value());
}

#define CHECK_ERROR(r, err) CHECK_FALSE(r.has_value()); CHECK_EQ(static_cast<int>(r.error()), static_cast<int>(err));


template<typename T>
void CHECK_READ_ERROR(u8lib::JsonReader& r, u8lib::u8string_view key, u8lib::JsonErrorCode code) {
	T value;
	auto result = r.read(key, value);
	CHECK_ERROR(result, code);
}

template<typename T>
void TestPrimitiveType(const T& value) {
	using namespace u8lib;

	JsonWriter writer(1);
	if (true) {
		const char8_t* key = u8"key";
		CHECK_OK(writer.start_object(u8""));

		CHECK_OK(writer.write(key, value));
		CHECK_OK(writer.end_object());
	}
	if (true) {
		auto json = writer.dump();
		LOG_INFO(u8"PRIMITIVE JSON: {}", json);

		if constexpr (std::is_same_v<T, u8string_view>) {
			JsonReader reader(json);
			CHECK_OK(reader.start_object(u8""));
			u8string_view result;
			CHECK_OK(reader.read(u8"key", result));
			CHECK_VALUE(result, value);
			CHECK_OK(reader.end_object());
		} else {
			JsonReader reader(json);
			CHECK_OK(reader.start_object(u8""));
			T result;
			LOG_INFO(u8"{}", typeid(result).name());
			CHECK_OK(reader.read(u8"key", result));
			CHECK_OK(reader.end_object());
			CHECK_VALUE(value, result);
		}
	}
}

template<typename T>
struct TestPrimitiveArray {
	template<typename... Args>
	TestPrimitiveArray(Args... params) {
		using namespace u8lib;
		JsonWriter writer(1);

		std::array<T, sizeof...(Args)> values = {T(params)...};

		if (true) {
			const char8_t* key = u8"key";
			CHECK_OK(writer.start_object(u8""));
			CHECK_OK(writer.write(values.size(), key, values));
			CHECK_OK(writer.end_array());
			CHECK_OK(writer.end_object());
		}
		if (true) {
			auto json = writer.dump();
			LOG_INFO(u8"PRIMITIVE ARRAY JSON: {}", json);
			JsonReader reader(json);

			CHECK_OK(reader.start_object(u8""));
			auto result = reader.start_array(u8"key");
			CHECK_OK(result);

			decltype(values) _values;
			CHECK_OK(reader.read(result.value(), _values.data()));
			CHECK_OK(reader.end_array());
			CHECK_OK(reader.end_object());

			for (size_t i = 0; i < std::size(values); i++) {
				CHECK_VALUE(values[i], _values[i]);
			}
		}
	}
};

TEST_CASE_FIXTURE(JSONTests, "primitive") {
	using namespace u8lib;

	TestPrimitiveType<bool>(true);
	TestPrimitiveType<bool>(false);

	TestPrimitiveType<int8_t>(-1);
	TestPrimitiveType<int16_t>(-1);
	TestPrimitiveType<int32_t>(-1);
	TestPrimitiveType<int64_t>(-1);

	TestPrimitiveType<uint8_t>(-1);
	TestPrimitiveType<uint16_t>(-2);
	TestPrimitiveType<uint32_t>(-3);
	TestPrimitiveType<uint64_t>(-4);

	TestPrimitiveType<float>(234.2f);
	TestPrimitiveType<double>(-3.01e-10);

	TestPrimitiveType<const char8_t*>(u8"😀emoji");
	TestPrimitiveType<u8string>(u8"不是哥们");
	TestPrimitiveType<u8string_view>(u8"SerdeTest");
}

TEST_CASE_FIXTURE(JSONTests, "array") {
	using namespace u8lib;

	TestPrimitiveArray<bool>(true, false);
	TestPrimitiveArray<int8_t>(-1, 1, 0);
	TestPrimitiveArray<int16_t>(-1, 1, 0);
	TestPrimitiveArray<int32_t>(-1, 1, 0);
	TestPrimitiveArray<int64_t>(-1, 1, 0);

	TestPrimitiveArray<uint8_t>(-1, 1, 0);
	TestPrimitiveArray<uint16_t>(-1, 1, 0);
	TestPrimitiveArray<uint32_t>(-1, 1, 0);
	TestPrimitiveArray<uint64_t>(-1, 1, 0);

	TestPrimitiveArray<float>(-100.4, 100.001);
	TestPrimitiveArray<double>(-100e10, 100e10);

	TestPrimitiveArray<const char8_t*>(u8"Text", u8"#@@!*&の");
	TestPrimitiveArray<u8string>(u8"12345", u8"😀emoji");
	TestPrimitiveArray<u8string_view>(u8"eef", u8"积极你太美");
}

TEST_CASE_FIXTURE(JSONTests, "read errors") {
	using namespace u8lib;

	SUBCASE("NoOpenScope") {
		CHECK_ERROR(JsonReader(u8"{}").end_array(), JsonErrorCode::NoOpenScope);
		CHECK_ERROR(JsonReader(u8"{}").end_object(), JsonErrorCode::NoOpenScope);
	}

	SUBCASE("ScopeTypeMismatch(start_array)") {
		auto obj_reader = JsonReader(u8"{ \"obj\": \"value\" }");
		CHECK_OK(obj_reader.start_object(u8""));
		CHECK_ERROR(obj_reader.start_array(u8"obj"), JsonErrorCode::ScopeTypeMismatch);
	}

	SUBCASE("ScopeTypeMismatch(end_objectAsArray)") {
		auto obj_reader = JsonReader(u8"{}");
		CHECK_OK(obj_reader.start_object(u8""));
		CHECK_ERROR(obj_reader.end_array(), JsonErrorCode::ScopeTypeMismatch);
	}

	SUBCASE("ScopeTypeMismatch(start_arrayAsObject)") {
		auto arr_reader = JsonReader(u8"{ \"arr\": [ 0, 1, 2, 3 ] }");
		CHECK_OK(arr_reader.start_object(u8""));
		CHECK_ERROR(arr_reader.start_object(u8"arr"), JsonErrorCode::ScopeTypeMismatch);
	}

	SUBCASE("ScopeTypeMismatch(StartPrimitiveAsObject/Array)") {
		auto value_reader = JsonReader(u8"{ \"value\": 123 }");
		CHECK_OK(value_reader.start_object(u8""));
		CHECK_ERROR(value_reader.start_object(u8"value"), JsonErrorCode::ScopeTypeMismatch);
		CHECK_ERROR(value_reader.start_array(u8"value"), JsonErrorCode::ScopeTypeMismatch);

		auto arr_reader = JsonReader(u8"{ \"arr\": [ 0, 1, 2, 3 ] }");
		CHECK_OK(arr_reader.start_object(u8""));
		auto result = arr_reader.start_array(u8"arr");
		CHECK_OK(result);
		CHECK_EQ(result.value(), 4);
		CHECK_ERROR(arr_reader.start_object(u8""), JsonErrorCode::ScopeTypeMismatch);
		CHECK_ERROR(arr_reader.start_array(u8""), JsonErrorCode::ScopeTypeMismatch);
	}

	SUBCASE("ScopeTypeMismatch(end_arrayAsObject)") {
		auto arr_reader = JsonReader(u8"{ \"arr\": [ 0, 1, 2, 3 ] }");
		CHECK_OK(arr_reader.start_object(u8""));

		auto result = arr_reader.start_array(u8"arr");
		CHECK_OK(result);
		CHECK_EQ(result.value(), 4);
		CHECK_ERROR(arr_reader.end_object(), JsonErrorCode::ScopeTypeMismatch);
	}

	SUBCASE("KeyNotFound(ObjectField)") {
		auto obj_reader = JsonReader(u8"{ \"key\": \"value\" }");
		CHECK_OK(obj_reader.start_object(u8""));
		CHECK_READ_ERROR<u8string>(obj_reader, u8"key_mismatch", JsonErrorCode::KeyNotFound);
		CHECK_ERROR(obj_reader.start_array(u8"key_mismatch"), JsonErrorCode::KeyNotFound);
		CHECK_ERROR(obj_reader.start_object(u8"key_mismatch"), JsonErrorCode::KeyNotFound);
	}

	SUBCASE("KeyNotFound(ArrayIndexOverflow)") {
		auto arr_reader = JsonReader(u8"{ \"arr\": [ 0 ] }");
		CHECK_OK(arr_reader.start_object(u8""));
		if (true) {
			auto result = arr_reader.start_array(u8"arr");
			CHECK_OK(result);
			CHECK_EQ(result.value(), 1);
		}

		if (true) {
			int32_t value;
			auto result = arr_reader.read(u8"", value);
			CHECK_OK(result);
			CHECK_EQ(value, 0);
		}
		CHECK_READ_ERROR<int32_t>(arr_reader, u8"", JsonErrorCode::KeyNotFound);
	}

	SUBCASE("EmptyObjectFieldKey") {
		auto obj_reader = JsonReader(u8"{ \"key\": \"value\" }");
		CHECK_OK(obj_reader.start_object(u8""));
		CHECK_ERROR(obj_reader.start_object(u8""), JsonErrorCode::EmptyObjectFieldKey);
		CHECK_ERROR(obj_reader.start_array(u8""), JsonErrorCode::EmptyObjectFieldKey);

		CHECK_READ_ERROR<bool>(obj_reader, u8"", JsonErrorCode::EmptyObjectFieldKey);
		CHECK_READ_ERROR<int32_t>(obj_reader, u8"", JsonErrorCode::EmptyObjectFieldKey);
		CHECK_READ_ERROR<int64_t>(obj_reader, u8"", JsonErrorCode::EmptyObjectFieldKey);
		CHECK_READ_ERROR<float>(obj_reader, u8"", JsonErrorCode::EmptyObjectFieldKey);
		CHECK_READ_ERROR<double>(obj_reader, u8"", JsonErrorCode::EmptyObjectFieldKey);
		CHECK_READ_ERROR<u8string>(obj_reader, u8"", JsonErrorCode::EmptyObjectFieldKey);
	}

	SUBCASE("ArrayElementWithKey") {
		auto arr_reader = JsonReader(u8"{ \"arr\": [ 0 ] }");
		CHECK_OK(arr_reader.start_object(u8""));
		auto result = arr_reader.start_array(u8"arr");
		CHECK_OK(result);
		CHECK_EQ(result.value(), 1);

		CHECK_READ_ERROR<bool>(arr_reader, u8"k", JsonErrorCode::ArrayElementWithKey);
		CHECK_READ_ERROR<int32_t>(arr_reader, u8"k", JsonErrorCode::ArrayElementWithKey);
		CHECK_READ_ERROR<int64_t>(arr_reader, u8"k", JsonErrorCode::ArrayElementWithKey);
		CHECK_READ_ERROR<float>(arr_reader, u8"k", JsonErrorCode::ArrayElementWithKey);
		CHECK_READ_ERROR<double>(arr_reader, u8"k", JsonErrorCode::ArrayElementWithKey);
		CHECK_READ_ERROR<u8string>(arr_reader, u8"k", JsonErrorCode::ArrayElementWithKey);
		CHECK_ERROR(arr_reader.start_object(u8"k"), JsonErrorCode::ArrayElementWithKey);
		CHECK_ERROR(arr_reader.start_array(u8"k"), JsonErrorCode::ArrayElementWithKey);
	}

	SUBCASE("RootObjectWithKey") {
		auto obj_reader = JsonReader(u8"{ \"key\": \"value\" }");
		CHECK_ERROR(obj_reader.start_object(u8"key"), JsonErrorCode::RootObjectWithKey);
	}
}

TEST_CASE_FIXTURE(JSONTests, "write errors") {
	using namespace u8lib;

	SUBCASE("NoOpenScope") {
		CHECK_ERROR(JsonWriter(1).end_array(), JsonErrorCode::NoOpenScope);
		CHECK_ERROR(JsonWriter(1).end_object(), JsonErrorCode::NoOpenScope);
	}

	SUBCASE("ScopeTypeMismatch") {
		auto obj_writer = JsonWriter(3);
		CHECK_OK(obj_writer.start_object(u8""));
		CHECK_OK(obj_writer.start_object(u8"obj"));
		CHECK_ERROR(obj_writer.end_array(), JsonErrorCode::ScopeTypeMismatch);

		auto arr_writer = JsonWriter(3);
		CHECK_OK(arr_writer.start_object(u8""));
		CHECK_OK(arr_writer.start_array(u8"arr"));
		CHECK_ERROR(arr_writer.end_object(), JsonErrorCode::ScopeTypeMismatch);
	}

	SUBCASE("EmptyObjectFieldKey") {
		auto obj_writer = JsonWriter(3);
		CHECK_OK(obj_writer.start_object(u8""));
		CHECK_ERROR(obj_writer.start_object(u8""), JsonErrorCode::EmptyObjectFieldKey);
		CHECK_ERROR(obj_writer.start_array(u8""), JsonErrorCode::EmptyObjectFieldKey);

		bool b = false;
		int32_t i32 = 0;
		int64_t i64 = 0;
		float f = 0.f;
		double d = 0.0;
		u8string s = u8"";
		CHECK_ERROR(obj_writer.write(u8"", b), JsonErrorCode::EmptyObjectFieldKey);
		CHECK_ERROR(obj_writer.write(u8"", i32), JsonErrorCode::EmptyObjectFieldKey);
		CHECK_ERROR(obj_writer.write(u8"", i64), JsonErrorCode::EmptyObjectFieldKey);
		CHECK_ERROR(obj_writer.write(u8"", f), JsonErrorCode::EmptyObjectFieldKey);
		CHECK_ERROR(obj_writer.write(u8"", d), JsonErrorCode::EmptyObjectFieldKey);
		CHECK_ERROR(obj_writer.write(u8"", s), JsonErrorCode::EmptyObjectFieldKey);
	}

	SUBCASE("ArrayElementWithKey") {
		auto arr_writer = JsonWriter(3);
		CHECK_OK(arr_writer.start_object(u8""));
		CHECK_OK(arr_writer.start_array(u8"arr"));
		CHECK_ERROR(arr_writer.start_object(u8"k"), JsonErrorCode::ArrayElementWithKey);
		CHECK_ERROR(arr_writer.start_array(u8"k"), JsonErrorCode::ArrayElementWithKey);

		bool b = false;
		int32_t i32 = 0;
		int64_t i64 = 0;
		float f = 0.f;
		double d = 0.0;
		u8string s = u8"";
		CHECK_ERROR(arr_writer.write(u8"k", b), JsonErrorCode::ArrayElementWithKey);
		CHECK_ERROR(arr_writer.write(u8"k", i32), JsonErrorCode::ArrayElementWithKey);
		CHECK_ERROR(arr_writer.write(u8"k", i64), JsonErrorCode::ArrayElementWithKey);
		CHECK_ERROR(arr_writer.write(u8"k", f), JsonErrorCode::ArrayElementWithKey);
		CHECK_ERROR(arr_writer.write(u8"k", d), JsonErrorCode::ArrayElementWithKey);
		CHECK_ERROR(arr_writer.write(u8"k", s), JsonErrorCode::ArrayElementWithKey);
	}

	SUBCASE("RootObjectWithKey") {
		auto obj_writer = JsonWriter(3);
		CHECK_ERROR(obj_writer.start_object(u8"key"), JsonErrorCode::RootObjectWithKey);
	}
}
