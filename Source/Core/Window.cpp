#include "Window.h"

#include "Core/Error.h"
#include "GLFW.h"
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"
#include "Utils/Logger.h"
#include "Utils/UiInputs.h"

namespace Voluma {
class ApiCallbacks {
   public:
    static void windowSizeCallback(GLFWwindow *pGlfwWindow, int width,
                                   int height) {
        // We also get here in case the window was minimized, so we need to
        // ignore it
        if (width == 0 || height == 0) {
            return;
        }

        Window *pWindow = (Window *)glfwGetWindowUserPointer(pGlfwWindow);
        if (pWindow != nullptr) {
            pWindow->resize(width, height); // Window callback is handled in
                                            // here
        }
    }

    static void keyboardCallback(GLFWwindow *pGlfwWindow, int key, int scanCode,
                                 int action, int modifiers) {
        KeyboardEvent event;
        if (prepareKeyboardEvent(key, action, modifiers, event)) {
            Window *pWindow = (Window *)glfwGetWindowUserPointer(pGlfwWindow);
            if (pWindow != nullptr) {
                pWindow->mpCallbacks->handleKeyboardEvent(event);
            }
        }
    }

    static void charInputCallback(GLFWwindow *pGlfwWindow, uint32_t input) {
        KeyboardEvent event;
        event.type = KeyboardEvent::Type::Input;
        event.codepoint = input;

        Window *pWindow = (Window *)glfwGetWindowUserPointer(pGlfwWindow);
        if (pWindow != nullptr) {
            pWindow->mpCallbacks->handleKeyboardEvent(event);
        }
    }

    static void mouseMoveCallback(GLFWwindow *pGlfwWindow, double mouseX,
                                  double mouseY) {
        Window *pWindow = (Window *)glfwGetWindowUserPointer(pGlfwWindow);
        if (pWindow != nullptr) {
            // Prepare the mouse data
            MouseEvent event;
            event.type = MouseEvent::Type::Move;
            event.pos = calcMousePos(mouseX, mouseY, pWindow->getMouseScale());
            event.screenPos = {mouseX, mouseY};
            event.wheelDelta = float2(0, 0);

            pWindow->mpCallbacks->handleMouseEvent(event);
        }
    }

    static void mouseButtonCallback(GLFWwindow *pGlfwWindow, int button,
                                    int action, int modifiers) {
        MouseEvent event;
        // Prepare the mouse data
        MouseEvent::Type type = (action == GLFW_PRESS)
                                    ? MouseEvent::Type::ButtonDown
                                    : MouseEvent::Type::ButtonUp;
        switch (button) {
            case GLFW_MOUSE_BUTTON_LEFT:
                event.type = type;
                event.button = Input::MouseButton::Left;
                break;
            case GLFW_MOUSE_BUTTON_MIDDLE:
                event.type = type;
                event.button = Input::MouseButton::Middle;
                break;
            case GLFW_MOUSE_BUTTON_RIGHT:
                event.type = type;
                event.button = Input::MouseButton::Right;
                break;
            default:
                // Other keys are not supported
                return;
        }

        Window *pWindow = (Window *)glfwGetWindowUserPointer(pGlfwWindow);
        if (pWindow != nullptr) {
            // Modifiers
            event.mods = getModifierFlags(modifiers);
            double x, y;
            glfwGetCursorPos(pGlfwWindow, &x, &y);
            event.pos = calcMousePos(x, y, pWindow->getMouseScale());

            pWindow->mpCallbacks->handleMouseEvent(event);
        }
    }

    static void mouseWheelCallback(GLFWwindow *pGlfwWindow, double scrollX,
                                   double scrollY) {
        Window *pWindow = (Window *)glfwGetWindowUserPointer(pGlfwWindow);
        if (pWindow != nullptr) {
            MouseEvent event;
            event.type = MouseEvent::Type::Wheel;
            double x, y;
            glfwGetCursorPos(pGlfwWindow, &x, &y);
            event.pos = calcMousePos(x, y, pWindow->getMouseScale());
            event.wheelDelta = (float2(float(scrollX), float(scrollY)));

            pWindow->mpCallbacks->handleMouseEvent(event);
        }
    }

    static void errorCallback(int errorCode, const char *pDescription) {
        // GLFW errors are always recoverable. Therefore we just log the error.
        logError("GLFW error {}: {}", errorCode, pDescription);
    }

    static void droppedFileCallback(GLFWwindow *pGlfwWindow, int count,
                                    const char **paths) {
        Window *pWindow = (Window *)glfwGetWindowUserPointer(pGlfwWindow);
        if (pWindow) {
            for (int i = 0; i < count; i++) {
                std::filesystem::path path(paths[i]);
                pWindow->mpCallbacks->handleDroppedFile(path);
            }
        }
    }

   private:
    static inline Input::Key glfwToVolumaKey(int glfwKey) {
        static_assert(GLFW_KEY_ESCAPE == 256,
                      "GLFW_KEY_ESCAPE is expected to be 256");
        static_assert((uint32_t)Input::Key::Escape >= 256,
                      "Input::Key::Escape is expected to be at least 256");

        if (glfwKey < GLFW_KEY_ESCAPE) {
            // Printable keys are expected to have the same value
            return (Input::Key)glfwKey;
        }

        switch (glfwKey) {
            case GLFW_KEY_ESCAPE:
                return Input::Key::Escape;
            case GLFW_KEY_ENTER:
                return Input::Key::Enter;
            case GLFW_KEY_TAB:
                return Input::Key::Tab;
            case GLFW_KEY_BACKSPACE:
                return Input::Key::Backspace;
            case GLFW_KEY_INSERT:
                return Input::Key::Insert;
            case GLFW_KEY_DELETE:
                return Input::Key::Del;
            case GLFW_KEY_RIGHT:
                return Input::Key::Right;
            case GLFW_KEY_LEFT:
                return Input::Key::Left;
            case GLFW_KEY_DOWN:
                return Input::Key::Down;
            case GLFW_KEY_UP:
                return Input::Key::Up;
            case GLFW_KEY_PAGE_UP:
                return Input::Key::PageUp;
            case GLFW_KEY_PAGE_DOWN:
                return Input::Key::PageDown;
            case GLFW_KEY_HOME:
                return Input::Key::Home;
            case GLFW_KEY_END:
                return Input::Key::End;
            case GLFW_KEY_CAPS_LOCK:
                return Input::Key::CapsLock;
            case GLFW_KEY_SCROLL_LOCK:
                return Input::Key::ScrollLock;
            case GLFW_KEY_NUM_LOCK:
                return Input::Key::NumLock;
            case GLFW_KEY_PRINT_SCREEN:
                return Input::Key::PrintScreen;
            case GLFW_KEY_PAUSE:
                return Input::Key::Pause;
            case GLFW_KEY_F1:
                return Input::Key::F1;
            case GLFW_KEY_F2:
                return Input::Key::F2;
            case GLFW_KEY_F3:
                return Input::Key::F3;
            case GLFW_KEY_F4:
                return Input::Key::F4;
            case GLFW_KEY_F5:
                return Input::Key::F5;
            case GLFW_KEY_F6:
                return Input::Key::F6;
            case GLFW_KEY_F7:
                return Input::Key::F7;
            case GLFW_KEY_F8:
                return Input::Key::F8;
            case GLFW_KEY_F9:
                return Input::Key::F9;
            case GLFW_KEY_F10:
                return Input::Key::F10;
            case GLFW_KEY_F11:
                return Input::Key::F11;
            case GLFW_KEY_F12:
                return Input::Key::F12;
            case GLFW_KEY_KP_0:
                return Input::Key::Keypad0;
            case GLFW_KEY_KP_1:
                return Input::Key::Keypad1;
            case GLFW_KEY_KP_2:
                return Input::Key::Keypad2;
            case GLFW_KEY_KP_3:
                return Input::Key::Keypad3;
            case GLFW_KEY_KP_4:
                return Input::Key::Keypad4;
            case GLFW_KEY_KP_5:
                return Input::Key::Keypad5;
            case GLFW_KEY_KP_6:
                return Input::Key::Keypad6;
            case GLFW_KEY_KP_7:
                return Input::Key::Keypad7;
            case GLFW_KEY_KP_8:
                return Input::Key::Keypad8;
            case GLFW_KEY_KP_9:
                return Input::Key::Keypad9;
            case GLFW_KEY_KP_DECIMAL:
                return Input::Key::KeypadDel;
            case GLFW_KEY_KP_DIVIDE:
                return Input::Key::KeypadDivide;
            case GLFW_KEY_KP_MULTIPLY:
                return Input::Key::KeypadMultiply;
            case GLFW_KEY_KP_SUBTRACT:
                return Input::Key::KeypadSubtract;
            case GLFW_KEY_KP_ADD:
                return Input::Key::KeypadAdd;
            case GLFW_KEY_KP_ENTER:
                return Input::Key::KeypadEnter;
            case GLFW_KEY_KP_EQUAL:
                return Input::Key::KeypadEqual;
            case GLFW_KEY_LEFT_SHIFT:
                return Input::Key::LeftShift;
            case GLFW_KEY_LEFT_CONTROL:
                return Input::Key::LeftControl;
            case GLFW_KEY_LEFT_ALT:
                return Input::Key::LeftAlt;
            case GLFW_KEY_LEFT_SUPER:
                return Input::Key::LeftSuper;
            case GLFW_KEY_RIGHT_SHIFT:
                return Input::Key::RightShift;
            case GLFW_KEY_RIGHT_CONTROL:
                return Input::Key::RightControl;
            case GLFW_KEY_RIGHT_ALT:
                return Input::Key::RightAlt;
            case GLFW_KEY_RIGHT_SUPER:
                return Input::Key::RightSuper;
            case GLFW_KEY_MENU:
                return Input::Key::Menu;
            default:
                return Input::Key::Unknown;
        }
    }

    static inline Input::ModifierFlags getModifierFlags(int modifiers) {
        // The GLFW mods should match the Input::ModifierFlags, but this is used
        // for now to be safe if it changes in the future.
        Input::ModifierFlags flags = Input::ModifierFlags::None;
        if (modifiers & GLFW_MOD_ALT) flags |= Input::ModifierFlags::Alt;
        if (modifiers & GLFW_MOD_CONTROL) flags |= Input::ModifierFlags::Ctrl;
        if (modifiers & GLFW_MOD_SHIFT) flags |= Input::ModifierFlags::Shift;
        return flags;
    }

    /**
     * GLFW reports modifiers inconsistently on different platforms.
     * To make modifiers consistent we check the key action and adjust
     * the modifiers due to changes from the current action.
     */
    static int fixGLFWModifiers(int modifiers, int key, int action) {
        int bit = 0;
        if (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT)
            bit = GLFW_MOD_SHIFT;
        if (key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL)
            bit = GLFW_MOD_CONTROL;
        if (key == GLFW_KEY_LEFT_ALT || key == GLFW_KEY_RIGHT_ALT)
            bit = GLFW_MOD_ALT;
        return (action == GLFW_RELEASE) ? modifiers & (~bit) : modifiers | bit;
    }

    static inline float2 calcMousePos(double xPos, double yPos,
                                      const float2 &mouseScale) {
        float2 pos = float2(float(xPos), float(yPos));
        pos *= mouseScale;
        return pos;
    }

    static inline bool prepareKeyboardEvent(int key, int action, int modifiers,
                                            KeyboardEvent &event) {
        if (key == GLFW_KEY_UNKNOWN) {
            return false;
        }

        modifiers = fixGLFWModifiers(modifiers, key, action);

        switch (action) {
            case GLFW_RELEASE:
                event.type = KeyboardEvent::Type::KeyReleased;
                break;
            case GLFW_PRESS:
                event.type = KeyboardEvent::Type::KeyPressed;
                break;
            case GLFW_REPEAT:
                event.type = KeyboardEvent::Type::KeyRepeated;
                break;
            default:
                logFatal("Invalid action");
                break;
        }
        event.key = glfwToVolumaKey(key);
        event.mods = getModifierFlags(modifiers);
        return true;
    }
};

void Window::shutdown() { glfwSetWindowShouldClose(mpGLFWWindow, 1); }

void Window::msgLoop() {
    // Samples often rely on a size change event as part of initialization
    // This would have happened from a WM_SIZE message when calling ShowWindow
    // on Win32
    mpCallbacks->handleWindowSizeChange();

    while (!glfwWindowShouldClose(mpGLFWWindow)) {
        pollForEvents();
        mpCallbacks->handleRenderFrame();
    }
}

void Window::pollForEvents() { glfwPollEvents(); }

void Window::updateWindowSize() {
    int32_t width, height;
    glfwGetWindowSize(mpGLFWWindow, &width, &height);

    VL_ASSERT(width > 0 && height > 0);

    mDesc.width = width;
    mDesc.height = height;
    mMouseScale.x = 1.0f / (float)mDesc.width;
    mMouseScale.y = 1.0f / (float)mDesc.height;
}

void Window::resize(uint32_t width, uint32_t height) {
    glfwSetWindowSize(mpGLFWWindow, width, height);

    // In minimized mode GLFW reports incorrect window size

    updateWindowSize();

    mpCallbacks->handleWindowSizeChange();
}

std::shared_ptr<Window> Window::create(const Desc &desc,
                                       ICallbacks *pCallbacks) {
    return std::shared_ptr<Window>(new Window(desc, pCallbacks));
}

Window::Window(const Desc &desc, ICallbacks *pCallbacks)
    : mDesc(desc),
      mMouseScale(1.0f / (float)desc.width, 1.0f / (float)desc.height),
      mpCallbacks(pCallbacks) {
    // Set error callback
    glfwSetErrorCallback(ApiCallbacks::errorCallback);

    // Init GLFW when first window is created.
    if (glfwInit() == GLFW_FALSE) logFatal("Failed to initialize GLFW.");

    // Create the window
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    uint32_t w = desc.width;
    uint32_t h = desc.height;

    if (desc.resizableWindow == false) {
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    }

    mpGLFWWindow = glfwCreateWindow(w, h, desc.title.c_str(), nullptr, nullptr);
    if (!mpGLFWWindow) {
        logFatal("Failed to create GLFW window.");
    }

#if VL_WINDOWS
    mNativeHandle = glfwGetWin32Window(mpGLFWWindow);
    VL_ASSERT(mNativeHandle);
#else
#error
#endif

    updateWindowSize();

    glfwSetWindowUserPointer(mpGLFWWindow, this);

    // Set callbacks
    glfwSetWindowSizeCallback(mpGLFWWindow, ApiCallbacks::windowSizeCallback);
    glfwSetKeyCallback(mpGLFWWindow, ApiCallbacks::keyboardCallback);
    glfwSetMouseButtonCallback(mpGLFWWindow, ApiCallbacks::mouseButtonCallback);
    glfwSetCursorPosCallback(mpGLFWWindow, ApiCallbacks::mouseMoveCallback);
    glfwSetScrollCallback(mpGLFWWindow, ApiCallbacks::mouseWheelCallback);
    glfwSetCharCallback(mpGLFWWindow, ApiCallbacks::charInputCallback);
    glfwSetDropCallback(mpGLFWWindow, ApiCallbacks::droppedFileCallback);

    glfwShowWindow(mpGLFWWindow);
    glfwFocusWindow(mpGLFWWindow);
}
Window::~Window() {
    glfwDestroyWindow(mpGLFWWindow);

    glfwTerminate();
}
} // namespace Voluma