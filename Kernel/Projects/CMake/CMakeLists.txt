cmake_minimum_required(VERSION 2.8)

set(KernelIncludeDir  ${KernelRoot} PARENT_SCOPE)


file(GLOB Sources
	${KernelRoot}/Sources/M_APM/*.c
	${KernelRoot}/Sources/I*.cpp
	${KernelRoot}/Sources/V*.cpp
	${KernelRoot}/Sources/XLinux*.cpp)
	
list(APPEND Sources
	${KernelRoot}/Sources/Base64Coder.cpp
	${KernelRoot}/Sources/MurmurHash.cpp)

list(REMOVE_ITEM Sources
	${KernelRoot}/Sources/VMacStackCrawl.cpp
	${KernelRoot}/Sources/VResource.cpp
	${KernelRoot}/Sources/VUnicodeResources.cpp
	${KernelRoot}/Sources/VWinStackCrawl.cpp)


include_directories(${IcuIncludeDir}
	${BsdIncludeDir}
	${UuidIncludeDir}
	${KernelRoot}/Sources/M_APM
	${XBoxRoot})


add_library(Kernel SHARED ${Sources})



target_link_libraries(Kernel Icu
	Bsd               # Needed by arc4random
	${CMAKE_DL_LIBS}  # Needed by dlopen and friends (used by KernelIPC)
	rt                # Needed by clock_gettime
	Uuid)             # Needed by uuid_generate
