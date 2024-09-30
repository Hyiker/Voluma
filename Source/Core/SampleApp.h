#pragma once
#include "Window.h"

namespace Voluma {
class SampleApp : public Window::ICallbacks {
public:
  SampleApp();

  virtual void handleWindowSizeChange() {}
  virtual void handleRenderFrame() {}
  virtual void handleKeyboardEvent(const KeyboardEvent &keyEvent) {}
  virtual void handleMouseEvent(const MouseEvent &mouseEvent) {}
  virtual void handleDroppedFile(const std::filesystem::path &path) {}

private:
  std::shared_ptr<Window> mpWindow;
};
} // namespace Voluma