package("slangd")
    set_homepage("https://github.com/shader-slang/slang")
    set_description("Making it easier to work with shaders")
    set_license("MIT")

    if is_plat("windows") then
        if is_arch("x64", "x86_64") then
            add_urls("https://github.com/shader-slang/slang/releases/download/v$(version)/slang-$(version)-windows-x86_64.zip")
            add_versions("2024.11.1", "b6253b3e02fa206a9c50341dec8a85ebc09be614bfdc6cd8184cddb34ca97240")
            add_versions("2024.13", "b31d7e4211d6358b8fb9411668fc1a8125693f05a2b3340221f7d379af32c1e5")
        end
    elseif is_plat("macosx") then
        if is_arch("arm64") then
            add_urls("https://github.com/shader-slang/slang/releases/download/v$(version)/slang-$(version)-macos-aarch64.zip")
            add_versions("2024.13", "dd5adabf8ac1d19bf341b94d92670e7499d8f9655a33bbb0bebf7033ba634957")
        end
    end

    on_install("windows|x64", "macosx|arm64", function (package)
        os.cp("include", package:installdir())
        if package:is_plat("windows") then
            os.trycp(path.join("bin", "*.dll"), package:installdir("bin"))
            os.trycp(path.join("lib", "*.lib"), package:installdir("lib"))
        elseif package:is_plat("macosx") then
            -- os.trycp(path.join("bin", "*.dll"), package:installdir("bin"))
            os.trycp(path.join("lib", "*.a"), package:installdir("lib"))
            os.trycp(path.join("lib", "*.dylib"), package:installdir("lib"))
            package:add("links", "slang")
        end
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

rule("slang")
    set_extensions(".slang", ".slangh")
    on_build_file(function (target, sourcefile, opt)
        import("core.project.depend")
        import("utils.progress") -- it only for v2.5.9, we need use print to show progress below v2.5.8

        -- make sure build directory exists
        os.mkdir(target:targetdir())

        -- replace .md with .html
        local targetfile = path.join(target:targetdir(), path.filename(sourcefile))

        -- only rebuild the file if its changed since last run
        depend.on_changed(function ()
            -- call pandoc to make a standalone html file from a markdown file
            os.trycp(sourcefile, targetfile)
            progress.show(opt.progress, "${color.build.object}slang %s", sourcefile)
        end, {files = sourcefile})
    end)
