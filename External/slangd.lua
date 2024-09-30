package("slangd")
    set_homepage("https://github.com/shader-slang/slang")
    set_description("Making it easier to work with shaders")
    set_license("MIT")

    if is_plat("windows") then
        if is_arch("x64", "x86_64") then
            add_urls("https://github.com/shader-slang/slang/releases/download/v$(version)/slang-$(version)-windows-x86_64.zip")
            add_versions("2024.11.1", "b6253b3e02fa206a9c50341dec8a85ebc09be614bfdc6cd8184cddb34ca97240")
        end
    end

    on_install("windows|x64", function (package)
        os.trycp(path.join("bin", "*.dll"), package:installdir("bin"))
        os.cp("include", package:installdir())
        os.trycp(path.join("lib", "*.lib"), package:installdir("lib"))
        package:addenv("PATH", "bin")
    end)
    
    on_test(function (package)
        assert(package:check_cxxsnippets({ test = [[
            #include <slang-com-ptr.h>
            #include <slang.h>

            void test() {
                Slang::ComPtr<slang::IGlobalSession> global_session;
                slang::createGlobalSession(global_session.writeRef());
            }
        ]] }, {configs = {languages = "c++17"}}))
    end)