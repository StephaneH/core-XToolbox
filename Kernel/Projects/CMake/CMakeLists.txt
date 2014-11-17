

set(KernelIncludeDir  ${KernelRoot} PARENT_SCOPE)


file(GLOB Sources
	${KernelRoot}/Sources/M_APM/*.c
	${KernelRoot}/Sources/I*.cpp
	${KernelRoot}/Sources/V*.cpp
	${KernelRoot}/Sources/XBSD*.cpp
	${KernelRoot}/Sources/XLinux*.cpp)
	
list(APPEND Sources
	${KernelRoot}/Sources/Base64Coder.cpp
	${KernelRoot}/Sources/MurmurHash.cpp)

list(REMOVE_ITEM Sources
	${KernelRoot}/Sources/VMacStackCrawl.cpp
	${KernelRoot}/Sources/VResource.cpp
	${KernelRoot}/Sources/VUnicodeResources.cpp
	${KernelRoot}/Sources/VWinStackCrawl.cpp)


add_library(Kernel SHARED ${Sources})
add_library(KernelStatic STATIC EXCLUDE_FROM_ALL ${Sources})




set_property(TARGET Kernel APPEND PROPERTY INCLUDE_DIRECTORIES "${IcuIncludeDir}")
set_property(TARGET Kernel APPEND PROPERTY INCLUDE_DIRECTORIES "${UuidIncludeDir}")
set_property(TARGET Kernel APPEND PROPERTY INCLUDE_DIRECTORIES "${BsdIncludeDir}")
set_property(TARGET Kernel APPEND PROPERTY INCLUDE_DIRECTORIES "${XBoxRoot}")
set_property(TARGET Kernel APPEND PROPERTY INCLUDE_DIRECTORIES "${KernelRoot}/Sources/M_APM")

set_property(TARGET KernelStatic APPEND PROPERTY INCLUDE_DIRECTORIES "${IcuIncludeDir}")
set_property(TARGET KernelStatic APPEND PROPERTY INCLUDE_DIRECTORIES "${XBoxRoot}")
set_property(TARGET KernelStatic APPEND PROPERTY INCLUDE_DIRECTORIES "${UuidSystemIncludeDir}")
set_property(TARGET KernelStatic APPEND PROPERTY INCLUDE_DIRECTORIES "${BsdSystemIncludeDir}")
set_property(TARGET KernelStatic APPEND PROPERTY INCLUDE_DIRECTORIES "${KernelRoot}/Sources/M_APM")
set_property(TARGET KernelStatic APPEND PROPERTY COMPILE_DEFINITIONS "XTOOLBOX_AS_STANDALONE=1")


target_link_libraries(Kernel Icu
	${BsdLibs}        # Needed by arc4random
	${CMAKE_DL_LIBS}  # Needed by dlopen and friends (used by KernelIPC)
	rt                # Needed by clock_gettime
	${UuidLibs})      # Needed by uuid_generate

target_link_libraries(KernelStatic IcuStatic
	${BsdSystemLibs}        # Needed by arc4random
	${CMAKE_DL_LIBS}  # Needed by dlopen and friends (used by KernelIPC)
	rt                # Needed by clock_gettime
	${UuidSystemLibs}) # Needed by uuid_generate



