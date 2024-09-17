add_rules("mode.debug", "mode.release", "mode.releasedbg")
set_policy("build.warning", true)
set_warnings("allextra")

function vl_target(name)
target(name)
    -- c latest + c++20
    set_languages("clatest", "cxx20")
    -- force clang-cl toolchain on windows
    if is_plat("windows") then
        set_toolchains("clang-cl")
    end
end

-- Third party libraries
includes("ext/vl_dcmtk.lua")
add_requires("fmt 11.0.2")
add_requires("vl_dcmtk 3.6.8", {configs = {share = true, libtiff = false}})

-- Voluma source code and tests
includes("src", "tests")
