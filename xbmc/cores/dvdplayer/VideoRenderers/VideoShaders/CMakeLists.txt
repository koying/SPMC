set(SOURCES 
  ConvolutionKernels.cpp

)

set(HEADERS 
  ConvolutionKernels.h

)

if(OPENGL_FOUND OR OPENGLES_FOUND)
  list(APPEND SOURCES 
                      VideoFilterShader.cpp
                      YUV2RGBShader.cpp)
  list(APPEND HEADERS 
                      VideoFilterShader.h
                      YUV2RGBShader.h)
endif()

core_add_library(droidVshaders)
