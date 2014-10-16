
if(USE_V8_ENGINE STREQUAL 1)
	message(STATUS "Build with V8 !")
	add_definitions(-DUSE_V8_ENGINE=1)
else()
	message(STATUS "Build with WebKit !")
	add_definitions(-DUSE_V8_ENGINE=0)
endif()

set(JavaScriptIncludeDir    ${JavaScriptRoot}/Sources PARENT_SCOPE)


file(GLOB Sources ${JavaScriptRoot}/Sources/*.cpp)


add_library(JavaScript SHARED ${Sources})


if(USE_V8_ENGINE STREQUAL 1)
	set(JsIncludeDir /home/guy/v8/v8/include)
else()
	set(JsIncludeDir ${JsCoreIncludeDir})
endif()

include_directories(${CurlIncludeDir}
	${Graphics}
	${IcuIncludeDir}
	${JavaScriptRoot}/Sources
	${JsIncludeDir}
	${XBoxRoot}
	${ServerNet}
	${JavaScriptRoot}/../../../Components/Main)


if(USE_V8_ENGINE STREQUAL 1)
	add_library(v8 SHARED IMPORTED)
	#add_dependencies(v8 JavaScriptV8)
	set_property(TARGET v8 PROPERTY IMPORTED_LOCATION /home/guy/v8/v8/out/x64.debug/lib.target/libv8.so)

	target_link_libraries(JavaScript ${CurlLibs} Graphics v8 Kernel KernelIPC ServerNet Xml ZLib)
else()
	target_link_libraries(JavaScript ${CurlLibs} Graphics JsCore Kernel KernelIPC ServerNet Xml ZLib)
endif()

