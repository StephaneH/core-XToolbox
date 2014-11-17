
set(XMLIncludeDir   ${XMLRoot} PARENT_SCOPE)


file(GLOB Sources ${XMLRoot}/Sources/*.cpp)




include_directories(${IcuIncludeDir}
  ${KernHeaders}
  ${XBoxRoot}
  ${XercesIncludeDir}
  ${XMLRoot}/Sources)


add_library(Xml SHARED ${Sources})
add_library(XmlStatic STATIC EXCLUDE_FROM_ALL ${Sources})


target_link_libraries(Xml Kernel KernelIPC Xerces)
