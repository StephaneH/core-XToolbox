

set(JavaScriptIncludeDir    ${JavaScriptRoot}/Sources CACHE INTERNAL "Javascript Library include dir")


file(GLOB Sources ${JavaScriptRoot}/Sources/*.cpp)

list(REMOVE_ITEM Sources
    ${JavaScriptRoot}/Sources/XWinEnvironmentVariables.cpp)

include_directories(
	${CurlIncludeDir}
	${Graphics}
	${IcuIncludeDir}
	${JavaScriptRoot}/Sources
	${JSEngineIncludeDir}
	${XBoxRoot}
	${ServerNet}
	#${JavaScriptRoot}/../../../Components/Main)
	)

add_library(JavaScript EXCLUDE_FROM_ALL SHARED ${Sources})

target_link_libraries(JavaScript
	${CurlLibs} Graphics ${JSEngineLibs} Kernel KernelIPC ServerNet Xml ${ZLibLibs})

add_dependencies(JavaScript  "${JSEngineDeps}")

