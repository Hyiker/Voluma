#pragma once
#include "Macros.h"

#define GLFW_INCLUDE_NONE

#ifdef VL_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#else
#error "TODO"
#endif