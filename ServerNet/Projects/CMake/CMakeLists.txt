set(ServerNetIncludeDir ${ServerNetRoot} ${ServerNetRoot}/Headers PARENT_SCOPE)


file(GLOB Sources ${ServerNetRoot}/Sources/*.cpp)

list(REMOVE_ITEM Sources ${ServerNetRoot}/Sources/SslStub.cpp
  ${ServerNetRoot}/Sources/XWinNetAddr.cpp
  ${ServerNetRoot}/Sources/XWinSocket.cpp)




include_directories(${IcuIncludeDir}
  ${ServerNetRoot}/Headers
  ${XBoxRoot})


add_library(ServerNet SHARED ${Sources})
add_library(ServerNetStatic STATIC EXCLUDE_FROM_ALL ${Sources})

set_property(TARGET ServerNet APPEND PROPERTY INCLUDE_DIRECTORIES "${OpenSSLIncludeDir}")
set_property(TARGET ServerNet APPEND PROPERTY INCLUDE_DIRECTORIES "${ZLibIncludeDir}")

set_property(TARGET ServerNetStatic APPEND PROPERTY INCLUDE_DIRECTORIES "${OpenSSLSystemIncludeDir}")
set_property(TARGET ServerNetStatic APPEND PROPERTY INCLUDE_DIRECTORIES "${ZLibSystemIncludeDir}")
set_property(TARGET ServerNetStatic APPEND PROPERTY COMPILE_DEFINITIONS "XTOOLBOX_AS_STANDALONE=1")


target_link_libraries(ServerNet ${OpenSSLLibs} Kernel KernelIPC Xml ${ZLibLibs})

target_link_libraries(ServerNetStatic ${OpenSSLSystemLibs} KernelStatic KernelIPCStatic XmlStatic ${ZLibSystemLibs})
