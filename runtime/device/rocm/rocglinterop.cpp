//
// Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.
//

#ifndef WITHOUT_HSA_BACKEND

#include "os/os.hpp"
#include "utils/debug.hpp"
#include "utils/flags.hpp"
#include "device/rocm/rocglinterop.hpp"

#if !defined(_WIN32)
#include <dlfcn.h>
#endif

namespace roc {

namespace MesaInterop {

#if !defined(_WIN32)
static PFNMESAGLINTEROPGLXQUERYDEVICEINFOPROC GlxInfo = nullptr;
static PFNMESAGLINTEROPGLXEXPORTOBJECTPROC GlxExport = nullptr;
static PFNMESAGLINTEROPEGLQUERYDEVICEINFOPROC EglInfo = nullptr;
static PFNMESAGLINTEROPEGLEXPORTOBJECTPROC EglExport = nullptr;
static MESA_INTEROP_KIND loadedGLAPITypes(MESA_INTEROP_NONE);
#endif

static const char* errorStrings[] = {"MESA_GLINTEROP_SUCCESS",
                                     "MESA_GLINTEROP_OUT_OF_RESOURCES",
                                     "MESA_GLINTEROP_OUT_OF_HOST_MEMORY",
                                     "MESA_GLINTEROP_INVALID_OPERATION",
                                     "MESA_GLINTEROP_INVALID_VALUE",
                                     "MESA_GLINTEROP_INVALID_DISPLAY",
                                     "MESA_GLINTEROP_INVALID_CONTEXT",
                                     "MESA_GLINTEROP_INVALID_TARGET",
                                     "MESA_GLINTEROP_INVALID_OBJECT",
                                     "MESA_GLINTEROP_INVALID_MIP_LEVEL",
                                     "MESA_GLINTEROP_UNSUPPORTED"};

bool Supported() {
#ifdef _WIN32
  return false;
#else
  return true;
#endif
}

// Returns true if the required subsystem is supported on the GL device.
// Must be called at least once, may be called multiple times.
bool Init(MESA_INTEROP_KIND Kind) {
#if defined(_WIN32)
  return false;
#else
  if (loadedGLAPITypes == MESA_INTEROP_NONE) {
    void* glxinfo = dlsym(RTLD_DEFAULT, "MesaGLInteropGLXQueryDeviceInfo");
    void* eglinfo = dlsym(RTLD_DEFAULT, "MesaGLInteropEGLQueryDeviceInfo");

    GlxInfo = (PFNMESAGLINTEROPGLXQUERYDEVICEINFOPROC)glxinfo;
    EglInfo = (PFNMESAGLINTEROPEGLQUERYDEVICEINFOPROC)eglinfo;

    GlxExport =
        (PFNMESAGLINTEROPGLXEXPORTOBJECTPROC)dlsym(RTLD_DEFAULT, "MesaGLInteropGLXExportObject");
    EglExport =
        (PFNMESAGLINTEROPEGLEXPORTOBJECTPROC)dlsym(RTLD_DEFAULT, "MesaGLInteropEGLExportObject");

    uint32_t ret = MESA_INTEROP_NONE;
    if (GlxInfo && GlxExport) ret |= MESA_INTEROP_GLX;
    if (EglInfo && EglExport) ret |= MESA_INTEROP_EGL;
    loadedGLAPITypes = MESA_INTEROP_KIND(ret);
  }

  return ((loadedGLAPITypes & Kind) == Kind);
#endif
}

bool GetInfo(mesa_glinterop_device_info& info, MESA_INTEROP_KIND Kind, const DisplayHandle display,
             const ContextHandle context) {
#ifdef _WIN32
  return false;
#else
  assert((loadedGLAPITypes & Kind) == Kind && "Requested interop API is not currently loaded.");
  int ret;
  switch (Kind) {
    case MESA_INTEROP_GLX:
      ret = GlxInfo(display.glxDisplay, context.glxContext, &info);
      break;
    case MESA_INTEROP_EGL:
      ret = EglInfo(display.eglDisplay, context.eglContext, &info);
      break;
    default:
      assert(false && "Invalid interop kind.");
      return false;
  }
  if (ret == MESA_GLINTEROP_SUCCESS) return true;
  if (ret < int(sizeof(errorStrings) / sizeof(errorStrings[0])))
    LogPrintfError("Mesa interop: GetInfo failed with \"%s\".\n", errorStrings[ret]);
  else
    LogError("Mesa interop: GetInfo failed with invalid error code.\n");
  return false;
#endif
}

bool Export(mesa_glinterop_export_in& in, mesa_glinterop_export_out& out, MESA_INTEROP_KIND Kind,
            const DisplayHandle display, const ContextHandle context) {
#ifdef _WIN32
  return false;
#else
  assert((loadedGLAPITypes & Kind) == Kind && "Requested interop API is not currently loaded.");
  int ret;
  switch (Kind) {
    case MESA_INTEROP_GLX:
      ret = GlxExport(display.glxDisplay, context.glxContext, &in, &out);
      break;
    case MESA_INTEROP_EGL:
      ret = EglExport(display.eglDisplay, context.eglContext, &in, &out);
      break;
    default:
      assert(false && "Invalid interop kind.");
      return false;
  }
  if (ret == MESA_GLINTEROP_SUCCESS) return true;
  if (ret < int(sizeof(errorStrings) / sizeof(errorStrings[0])))
    LogPrintfError("Mesa interop: Export failed with \"%s\".\n", errorStrings[ret]);
  else
    LogError("Mesa interop: Export failed with invalid error code.\n");
  return false;
#endif
}
}
}

#endif  // WITHOUT_HSA_BACKEND
