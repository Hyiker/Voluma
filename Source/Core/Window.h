#pragma once
#include <cstdint>
#include <filesystem>
#include <string>

#include "Core/Math.h"
#include "Macros.h"

struct GLFWwindow;
namespace Voluma {

struct KeyboardEvent;
struct MouseEvent;

#if VL_WINDOWS
using WindowHandle = void *;
#else
using WindowHandle = struct objc_object *;
#endif

class VL_API Window {
   public:
    /**
     * Window configuration configuration
     */
    struct Desc {
        uint32_t width = 1920;  ///< The width of the client area size.
        uint32_t height = 1080; ///< The height of the client area size.
        std::string title = "Voluma Viz"; ///< Window title.
        bool resizableWindow = true; ///< Allow the user to resize the window.
        bool enableVSync = false;    ///< Controls vertical-sync.
    };

    class ICallbacks {
       public:
        virtual void handleWindowSizeChange() = 0;
        virtual void handleRenderFrame() = 0;
        virtual void handleKeyboardEvent(const KeyboardEvent &keyEvent) = 0;
        virtual void handleMouseEvent(const MouseEvent &mouseEvent) = 0;
        virtual void handleDroppedFile(const std::filesystem::path &path) = 0;
    };

    Window(const Window &) = delete;

    Window(Window &&other) noexcept;

    void updateWindowSize();

    void shutdown();

    void msgLoop();

    void pollForEvents();

    void resize(uint32_t width, uint32_t height);

    WindowHandle getNativeHandle() const { return mNativeHandle; }

    static std::shared_ptr<Window> create(const Desc &desc,
                                          ICallbacks *pCallbacks);

    Window &operator=(const Window &) = delete;

    Window &operator=(Window &&) noexcept;

    ~Window();

   private:
    friend class ApiCallbacks;
    Window(const Desc &desc, ICallbacks *pCallbacks);

    Desc mDesc;
    GLFWwindow *mpGLFWWindow;
    WindowHandle mNativeHandle;
    float2 mMouseScale;
    const float2 &getMouseScale() const { return mMouseScale; }
    ICallbacks *mpCallbacks = nullptr;
};
} // namespace Voluma
