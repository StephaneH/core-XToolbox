

set(ServerNetIncludeDir ${ServerNetRoot} ${ServerNetRoot}/Headers PARENT_SCOPE)


file(GLOB Sources ${ServerNetRoot}/Sources/*.cpp)

list(REMOVE_ITEM Sources ${ServerNetRoot}/Sources/SslStub.cpp
  ${ServerNetRoot}/Sources/XWinNetAddr.cpp
  ${ServerNetRoot}/Sources/XWinSocket.cpp)


add_library(ServerNet SHARED ${Sources})


include_directories(${IcuIncludeDir}
  ${ServerNetRoot}/Headers
  ${XBoxRoot}
  ${OpenSSLIncludeDir}
  ${ZLibIncludeDir})


target_link_libraries(ServerNet ${OpenSSLLibs} Kernel KernelIPC Xml ZLib)
