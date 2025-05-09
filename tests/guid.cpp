#include <doctest/doctest.h>

#include <u8lib/guid.hpp>

TEST_CASE("guid") {
	using namespace u8lib;
	constexpr auto g1 = guid_t::from_string(u8"1d29de69-b2b0-44f6-a1aa-4acf070bf8bb");
	constexpr auto g2 = guid_t::from_string(u8"{1d29de69-b2b0-44f6-a1aa-4acf070bf8bb}");
	CHECK_EQ(g1.value(), g2.value());
	CHECK_EQ(format(u8"{}", g1.value()), u8"1d29de69-b2b0-44f6-a1aa-4acf070bf8bb");
}
