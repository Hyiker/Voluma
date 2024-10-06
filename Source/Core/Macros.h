#pragma once

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
#define VL_WINDOWS 1
#elif defined(__APPLE__) || defined(__MACH__)
#define VL_MACOSX 1
#else
#define VL_LINUX 1
#endif

#ifdef __clang__
#define VL_CLANG
#elif defined(__GNUC__) || defined(__GNUG__)
#define VL_GCC
#elif defined(_MSC_VER)
#define VL_MSVC
#endif

// dynamic library import/export
#if defined(VL_WINDOWS)
#define VL_API_EXPORT __declspec(dllexport)
#define VL_API_IMPORT __declspec(dllimport)
#elif defined(VL_LINUX) || defined(VL_MACOSX)
#define VL_API_EXPORT __attribute__((visibility("default")))
#define VL_API_IMPORT
#else
#error "Unsupported platform"
#endif

#ifdef VL_SHARED
#ifdef VL_MODULE
#define VL_API VL_API_EXPORT
#else
#define VL_API VL_API_IMPORT
#endif
#else
#define VL_API
#endif

#define VL_FMT(cls)                                                   \
    template <>                                                       \
    class fmt::formatter<cls> : formatter<std::string> {              \
       public:                                                        \
        constexpr auto parse(format_parse_context& ctx) {             \
            return ctx.begin();                                       \
        }                                                             \
        auto format(const cls& d, format_context& ctx) const {        \
            return formatter<std::string>::format(d.toString(), ctx); \
        }                                                             \
    };
