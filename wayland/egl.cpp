#include <wayland/egl.hpp>

namespace egl {

const std::error_category& category() {
  static const struct : std::error_category {
    const char* name() const noexcept override { return "EGL"; }
    std::string message(int cond) const override {
      switch (cond) {
      case EGL_SUCCESS:
        return "Success";
      case EGL_NOT_INITIALIZED:
        return "EGL is not initialized, or could not be initialized, for the "
               "specified EGL display connection.";
      case EGL_BAD_ACCESS:
        return "EGL cannot access a requested resource (for example a context "
               "is bound in another thread).";
      case EGL_BAD_ALLOC:
        return "EGL failed to allocate resources for the requested operation.";
      case EGL_BAD_ATTRIBUTE:
        return "An unrecognized attribute or attribute value was passed in the "
               "attribute list.";
      case EGL_BAD_CONTEXT:
        return "An EGLContext argument does not name a valid EGL rendering "
               "context.";
      case EGL_BAD_CONFIG:
        return "An EGLConfig argument does not name a valid EGL frame buffer "
               "configuration.";
      case EGL_BAD_CURRENT_SURFACE:
        return "The current surface of the calling thread is a window, pixel "
               "buffer or pixmap that is no longer valid.";
      case EGL_BAD_DISPLAY:
        return "An EGLDisplay argument does not name a valid EGL display "
               "connection.";
      case EGL_BAD_SURFACE:
        return "An EGLSurface argument does not name a valid surface (window, "
               "pixel buffer or pixmap) configured for "
               "GL rendering.";
      case EGL_BAD_MATCH:
        return "Arguments are inconsistent (for example, a valid context "
               "requires buffers not supplied by a valid "
               "surface).";
      case EGL_BAD_PARAMETER:
        return "One or more argument values are invalid.";
      case EGL_BAD_NATIVE_PIXMAP:
        return "A NativePixmapType argument does not refer to a valid native "
               "pixmap.";
      case EGL_BAD_NATIVE_WINDOW:
        return "A NativeWindowType argument does not refer to a valid native "
               "window.";
      case EGL_CONTEXT_LOST:
        return "A power management event has occurred. The application must "
               "destroy all contexts and reinitialise "
               "OpenGL ES state and objects to continue rendering.";
      }
      return "Unknown error " + std::to_string(cond);
    }
  } instance;
  return instance;
}

} // namespace egl
