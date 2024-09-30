#pragma once
#include "Core/Enum.h"
#include "Core/Math.h"

namespace Voluma {
namespace Input {
/**
 * Enum for the mouse buttons. Used for checking mouse button state in the
 * InputState class.
 */
enum class MouseButton : uint32_t { Left, Middle, Right, Count };

/**
 * Flags for the different modifiers.
 */
enum class ModifierFlags : uint32_t { None = 0, Shift = 1, Ctrl = 2, Alt = 4 };
VL_ENUM_FLAG(ModifierFlags);

/**
 * Enum for the different modifiers. Used for checking modifier state in the
 * InputState class and for checking modifier status on the KeyboardEvent. For
 * that to work these needs to have the same values as their flags.
 */
enum class Modifier : uint32_t {
  Shift = (uint32_t)ModifierFlags::Shift,
  Ctrl = (uint32_t)ModifierFlags::Ctrl,
  Alt = (uint32_t)ModifierFlags::Alt
};

/**
 * Use this enum to find out which key was pressed. Alpha-numeric keys use their
 * uppercase ASCII code, so you can use that as well.
 */
enum class Key : uint32_t {
  // Key codes 0..255 are reserved for ASCII codes.
  Space = ' ',
  Apostrophe = '\'',
  Comma = ',',
  Minus = '-',
  Period = '.',
  Slash = '/',
  Key0 = '0',
  Key1 = '1',
  Key2 = '2',
  Key3 = '3',
  Key4 = '4',
  Key5 = '5',
  Key6 = '6',
  Key7 = '7',
  Key8 = '8',
  Key9 = '9',
  Semicolon = ';',
  Equal = '=',
  A = 'A',
  B = 'B',
  C = 'C',
  D = 'D',
  E = 'E',
  F = 'F',
  G = 'G',
  H = 'H',
  I = 'I',
  J = 'J',
  K = 'K',
  L = 'L',
  M = 'M',
  N = 'N',
  O = 'O',
  P = 'P',
  Q = 'Q',
  R = 'R',
  S = 'S',
  T = 'T',
  U = 'U',
  V = 'V',
  W = 'W',
  X = 'X',
  Y = 'Y',
  Z = 'Z',
  LeftBracket = '[',
  Backslash = '\\',
  RightBracket = ']',
  GraveAccent = '`',

  // Special keys start at key code 256.
  Escape = 256,
  Tab,
  Enter,
  Backspace,
  Insert,
  Del,
  Right,
  Left,
  Down,
  Up,
  PageUp,
  PageDown,
  Home,
  End,
  CapsLock,
  ScrollLock,
  NumLock,
  PrintScreen,
  Pause,
  F1,
  F2,
  F3,
  F4,
  F5,
  F6,
  F7,
  F8,
  F9,
  F10,
  F11,
  F12,
  Keypad0,
  Keypad1,
  Keypad2,
  Keypad3,
  Keypad4,
  Keypad5,
  Keypad6,
  Keypad7,
  Keypad8,
  Keypad9,
  KeypadDel,
  KeypadDivide,
  KeypadMultiply,
  KeypadSubtract,
  KeypadAdd,
  KeypadEnter,
  KeypadEqual,
  LeftShift,
  LeftControl,
  LeftAlt,
  LeftSuper, // Windows key on windows
  RightShift,
  RightControl,
  RightAlt,
  RightSuper, // Windows key on windows
  Menu,
  Unknown, // Any unknown key code

  Count,
};
} // namespace Input

/**
 * Abstracts mouse messages
 */
struct MouseEvent {
  /**
   * Message Type
   */
  enum class Type {
    ButtonDown, ///< Mouse button was pressed
    ButtonUp,   ///< Mouse button was released
    Move,       ///< Mouse cursor position changed
    Wheel       ///< Mouse wheel was scrolled
  };

  Type type;  ///< Event Type.
  float2 pos; ///< Normalized coordinates x,y in range [0, 1]. (0,0) is the
              ///< top-left corner of the window.
  float2 screenPos;  ///< Screen-space coordinates in range [0, clientSize].
                     ///< (0,0) is the top-left corner of the window.
  float2 wheelDelta; ///< If the current event is CMouseEvent#Type#Wheel, the
                     ///< change in wheel scroll. Otherwise zero.
  Input::ModifierFlags mods; ///< Keyboard modifier flags. Only valid if the
                             ///< event Type is one the button events
  Input::MouseButton button; ///< Which button was active. Only valid if the
                             ///< event Type is ButtonDown or ButtonUp.
};

struct KeyboardEvent {
  /**
   * Keyboard event Type
   */
  enum class Type {
    KeyPressed,  ///< Key was pressed.
    KeyReleased, ///< Key was released.
    KeyRepeated, ///< Key is repeatedly down.
    Input        ///< Character input
  };

  Type type;                 ///< The event type
  Input::Key key;            ///< The last key that was pressed/released
  Input::ModifierFlags mods; ///< Keyboard modifier flags
  uint32_t codepoint = 0; ///< UTF-32 codepoint from GLFW for Input event types

  bool hasModifier(Input::Modifier mod) const {
    return isSet(mods, (Input::ModifierFlags)mod);
  }
};

enum class GamepadButton : uint32_t {
  A,
  B,
  X,
  Y,
  LeftBumper,
  RightBumper,
  Back,
  Start,
  Guide,
  LeftThumb,
  RightThumb,
  Up,
  Right,
  Down,
  Left,

  Count,
};

struct GamepadEvent {
  enum class Type {
    ButtonDown,
    ButtonUp,
    Connected,
    Disconnected,
  };

  Type type;
  GamepadButton button;
};

} // namespace Voluma