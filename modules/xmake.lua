-- 静态链接有 bug，详见 module manager 板块
add_requires("mimalloc", { configs = { shared = true } })

target("u8lib")
do
    set_kind("$(kind)")
    set_group("01.libraries")
    add_rules("c++.unity_build", { batchsize = 16 })

    add_deps("compile-flags", { public = true })
    add_packages("mimalloc")

    add_files("private/*.cpp")
    add_includedirs("public", { public = true })
    add_headerfiles("public/(**)")

    after_load(function(target, opt)
        import("core.project.project")
        if (target:get("kind") == "shared") then
            target:add("defines", "U8LIB_DLL", { public = true })
        end
    end)

    if (is_os("windows")) then
        -- __imp_CoCreateGuid
        add_syslinks("Ole32", { public = true })
    end
    if (is_os("macosx")) then
        add_frameworks("CoreFoundation", { public = true })
    end

    target_end()
end
