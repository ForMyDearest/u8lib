add_requires("doctest")
add_requires("vld")

function TEST(name)
    target(name)
    do
        set_kind("binary")
        set_group("unit_test")
        add_deps("u8lib")
        add_packages("doctest")
        add_files(name .. ".cpp")
        add_defines("DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN")
    end
end

TEST("base")
TEST("string_view")
TEST("string")
TEST("format")
TEST("json")
TEST("guid")

target("logger")
do
    set_kind("binary")
    add_deps("u8lib")
    add_files("logger.cpp")
end