set(SOURCES 
  OverlayRendererUtil.cpp
  RenderCapture.cpp
  OverlayRendererGUI.cpp
  OverlayRenderer.cpp
  RenderFlags.cpp
  RenderManager.cpp
  BaseRenderer.cpp

)

set(HEADERS 
  RenderFlags.h
  OverlayRendererGUI.h
  RenderManager.h
  OverlayRenderer.h
  RenderCapture.h
  OverlayRendererUtil.h
  BaseRenderer.h
  RenderFormats.h

)

if(CORE_SYSTEM_NAME STREQUAL windows)
  list(APPEND SOURCES WinRenderer.cpp
                      OverlayRendererDX.cpp)
  list(APPEND HEADERS WinRenderer.h
                      OverlayRendererDX.h)
endif()

if(OPENGL_FOUND)
  list(APPEND SOURCES OverlayRendererGL.cpp
                      LinuxRendererGL.cpp
  )
  list(APPEND HEADERS OverlayRendererGL.h
                      LinuxRendererGL.h
  )
endif()

if(OPENGLES_FOUND)
  list(APPEND SOURCES LinuxRendererGLES.cpp
                      OverlayRendererGL.cpp
  )
  list(APPEND HEADERS LinuxRendererGLES.h
                      OverlayRendererGL.h
  )
endif()

if(ARCH MATCHES arm)
  list(APPEND SOURCES yuv2rgb.neon.S)
  list(APPEND HEADERS yuv2rgb.neon.h)
endif()

if(CORE_SYSTEM_NAME STREQUAL windows)
  list(APPEND SOURCES DXVAHD.cpp)
  list(APPEND HEADERS DXVAHD.h)
endif()


if(MMAL_FOUND)
  list(APPEND SOURCES MMALRenderer.cpp)
  list(APPEND HEADERS MMALRenderer.h)
endif()


core_add_library(videorenderers)
