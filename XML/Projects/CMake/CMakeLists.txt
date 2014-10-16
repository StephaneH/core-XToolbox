
set(XMLIncludeDir   ${XMLRoot} PARENT_SCOPE)


file(GLOB Sources ${XMLRoot}/Sources/*.cpp)


add_library(Xml SHARED ${Sources})


include_directories(${IcuIncludeDir}
  ${KernHeaders}
  ${XBoxRoot}
  ${XercesIncludeDir}
  ${XMLRoot}/Sources)


target_link_libraries(Xml Kernel KernelIPC Xerces)
