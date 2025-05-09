#include <doctest/doctest.h>
#include <u8lib/format.hpp>

#include <string>
#include <iterator>

struct Person {
	std::string name{"hhh"};
	int age{18};
	bool man{false};
};

namespace u8lib
{
	template<>
	struct formatter<Person> : formatter<const char*> {
		using base = formatter<const char*>;

		context::iterator format(const Person& value, context& ctx) const {
			return base::format(
				std::format("{} {}'s age is {}",
							value.man ? "Male" : "Female",
							value.name,
							value.age
				).data(), ctx);
		}
	};
}

TEST_CASE("sakura") {
	using namespace u8lib;
	// escaped braces
	CHECK_EQ(u8string_view{ u8"{}" }, format(u8"{{}}"));

	// integer
	CHECK_EQ(u8string_view{ u8"0" }, format(u8"{}", 0));
	CHECK_EQ(u8string_view{ u8"00255" }, format(u8"{:05d}", 255));
	CHECK_EQ(u8string_view{ u8"ff" }, format(u8"{:x}", 255));
	CHECK_EQ(u8string_view{ u8"-0xff" }, format(u8"{:#x}", -255));
	CHECK_EQ(u8string_view{ u8"_1762757171" }, format(u8"_{}", 1762757171ull));

	// float
	CHECK_EQ(u8string_view{ u8"3.14" }, format(u8"{}", 3.14f));
	CHECK_EQ(u8string_view{ u8"3.1" }, format(u8"{:.1f}", 3.14f));
	CHECK_EQ(u8string_view{ u8"-3.14000" }, format(u8"{:.5f}", -3.14f));
	CHECK_EQ(u8string_view{ u8"-99.999999999" }, format(u8"{}", -99.999999999));
	CHECK_EQ(u8string_view{ u8"60.004" }, format(u8"{}", 60.004));
	CHECK_EQ(u8string_view{ u8"inf" }, format(u8"{}", std::numeric_limits<float>::infinity()));
	CHECK_EQ(u8string_view{ u8"-inf" }, format(u8"{}", -std::numeric_limits<double>::infinity()));
	CHECK_EQ(u8string_view{ u8"nan" }, format(u8"{}", std::numeric_limits<float>::quiet_NaN()));

	// pointer
	CHECK_EQ(u8string_view{ u8"nullptr" }, format(u8"{}", nullptr));
	CHECK_EQ(u8string_view{ u8"0x75bcd15" }, format(u8"{}", reinterpret_cast<void*>(123456789)));

	// string
	CHECK_EQ(u8string_view{ u8"繁星明 😀😀" },
			format(u8"{}{}明{}{}", u8"繁",
				std::u8string_view{ u8"星" },
				std::u8string_view{ u8" 😀" },
				std::u8string_view{ u8"😀" }));

	CHECK_EQ(u8string_view{ u8"Female hhh's age is 18" }, format(u8"{}", Person{}));

	char c = 120;
	CHECK_EQ(format(u8"{:6}", 42), u8"    42");
	CHECK_EQ(format(u8"{:6}", 'x'), u8"x     ");
	CHECK_EQ(format(u8"{:*<6}", 'x'), u8"x*****");
	CHECK_EQ(format(u8"{:*>6}", 'x'), u8"*****x");
	CHECK_EQ(format(u8"{:*^6}", 'x'), u8"**x***");
	CHECK_EQ(format(u8"{:6d}", c), u8"   120");
	CHECK_EQ(format(u8"{:6}", true), u8"true  ");
}
