#pragma once
#include "Macros.h"

#define GLFW_INCLUDE_NONE

#if VL_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#elif VL_MACOSX
#define GLFW_EXPOSE_NATIVE_COCOA
#endif

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>