

set(GraphicsIncludeDir	${GraphicsRoot}/Headers PARENT_SCOPE)


  
list(APPEND xbox_graphics_src 
  ${GraphicsRoot}/Sources/V4DPicture.cpp
  ${GraphicsRoot}/Sources/V4DPictureData.cpp
  ${GraphicsRoot}/Sources/V4DPictureDataSource.cpp
  ${GraphicsRoot}/Sources/V4DPictureDecoder.cpp
  ${GraphicsRoot}/Sources/VAffineTransform.cpp
  ${GraphicsRoot}/Sources/VRect.cpp
  ${GraphicsRoot}/Sources/VColor.cpp
  ${GraphicsRoot}/Sources/V4DPictureTools.cpp

  #   ${GraphicsRoot}/Sources/
  #   ${GraphicsRoot}/Sources/
  #   ${GraphicsRoot}/Sources/
  #   ${GraphicsRoot}/Sources/
  )


include_directories(${IcuIncludeDir} ${KernelIncludeDir} ${XBoxRoot})


#add_library(GraphicsStatic STATIC EXCLUDE_FROM_ALL ${xbox_graphics_src})
add_library(Graphics SHARED ${xbox_graphics_src})




target_link_libraries(Graphics Kernel KernelIPC)
#target_link_libraries(GraphicsStatic KernelStatic KernelIPCStatic)
