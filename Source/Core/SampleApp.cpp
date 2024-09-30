#include "SampleApp.h"

namespace Voluma {
SampleApp::SampleApp() {
  Window::Desc desc;
  mpWindow = Window::create(desc, this);
  
  mpWindow->msgLoop();
}
} // namespace Voluma