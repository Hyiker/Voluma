add_rules("mode.debug", "mode.release", "mode.releasedbg")
set_policy("build.warning", true)
set_warnings("allextra")

function vl_target(name)
target(name)
    add_rules("slang")
    -- c latest + c++20
    set_languages("clatest", "cxx20")
    -- enforce encoding
    set_encodings("utf-8")
    -- force clang-cl toolchain on windows
    if is_plat("windows") then
        set_toolchains("clang-cl")
        add_defines("NOMINMAX", "WIN32_LEAN_AND_MEAN", "_CRT_SECURE_NO_WARNINGS")
    elseif is_plat("macosx") then 
        add_links("slang", "gfx") -- TODO: find a btter way
    end
end

-- Third party libraries
includes("External/vl_dcmtk.lua", "External/slangd.lua")
add_requires("fmt 11.0.2")
add_requires("lodepng")
add_requires("tinyexr v1.0.9")
add_requires("glfw 3.4", {
    configs = {glfw_include = "system"}})
add_requires("imgui v1.91.1")
if is_plat("windows") then 
add_requireconfs("imgui",  {configs = {dx12 = true, glfw = true}})
else 
add_requireconfs("imgui",  {configs = {glfw = true}})
end
add_requires("slangd 2024.13")
add_requires("glm 1.0.1", {
    configs = {header_only = false, cxx_standard = "20"}})
add_requires("vl_dcmtk 3.6.8", {
    configs = {shared = true, libtiff = false,
    libpng = false, openssl = false, libxml2 = false,
    zlib = false, libsndfile = false, libiconv = false,
    openjpeg = false, runtimes = "MD"}})

-- Voluma source code and tests
includes("Source", "Tests")
