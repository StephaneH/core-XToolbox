
set(KernelIPCIncludeDir   ${KernelIPCRoot} PARENT_SCOPE)


file(GLOB Sources
	${KernelIPCRoot}/Sources/V*.cpp
	${KernelIPCRoot}/Sources/V*.hxx
	${KernelIPCRoot}/Sources/V*.h
	${KernelIPCRoot}/Sources/XLinux*.cpp
	${KernelIPCRoot}/Sources/XLinux*.h
	${KernelIPCRoot}/Sources/XLinux*.hxx
	${KernelIPCRoot}/Sources/XPosix*.cpp
	${KernelIPCRoot}/Sources/XPosix*.h
	${KernelIPCRoot}/Sources/XPosix*.hxx
	${KernelIPCRoot}/Sources/XSysV*.cpp
	${KernelIPCRoot}/Sources/XSysV*.h
	${KernelIPCRoot}/Sources/XSysV*.hxx
)

# This file defines the components entry point ; It must be compiled
# with each component but is not part of KernelIpc shared library
list(REMOVE_ITEM Sources ${KernelIPCRoot}/Sources/VComponentLibrary.cpp)

set(ComponentEntryPoint  ${KernelIPCRoot}/Sources/VComponentLibrary.cpp PARENT_SCOPE)


add_library(KernelIPC SHARED ${Sources})


include_directories(${IcuIncludeDir} ${XBoxRoot})

target_link_libraries(KernelIPC Kernel)

if (HAVE_CAP_NG)
	target_link_libraries(KernelIPC Kernel cap-ng)
endif()

