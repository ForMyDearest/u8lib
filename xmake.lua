set_project("u8lib")

set_warnings("all")
set_policy("build.ccache", true)
set_policy("build.warning", true)
set_policy("check.auto_ignore_flags", true)
set_policy("package.install_locally", false)

set_languages("cxx23", "c11")
add_rules("mode.debug", "mode.release", "mode.releasedbg")

if (is_os("windows")) then
    add_defines("UNICODE", "NOMINMAX", "_WINDOWS", "WIN32_LEAN_AND_MEAN")
    add_defines("_CRT_SECURE_NO_WARNINGS")
    if (is_mode("release")) then
        set_runtimes("MD")
    else
        set_runtimes("MDd")
    end
end

includes("xmake/compile_flags.lua")
includes("modules/xmake.lua")
includes("tests/xmake.lua")