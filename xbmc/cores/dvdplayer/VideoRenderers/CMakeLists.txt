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


core_add_library(droidVrenderers)
