
#set(CMAKE_CXX_FLAGS_DEBUG   "-g -ferror-limit=1")

set(GraphicsIncludeDir	${GraphicsRoot}/Headers PARENT_SCOPE)


#file(GLOB Sources ${GraphicsRoot}/Sources/V*.cpp)
  
list(APPEND Sources 
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


#list(REMOVE_ITEM Sources ${GraphicsRoot}/Sources/V4DPictureProxyCache.cpp
#  ${GraphicsRoot}/Sources/V4DPictureTools.cpp)


add_library(Graphics SHARED ${Sources})


include_directories(${IcuIncludeDir} ${KernelIncludeDir} ${XBoxRoot})


target_link_libraries(Graphics Kernel KernelIPC)
