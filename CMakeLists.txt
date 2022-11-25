cmake_minimum_required(VERSION 3.18)

project(
	x64dbg-eazutil
	LANGUAGES CXX C
	DESCRIPTION "x64dbg Plugin for Eazfuscator"
	VERSION 0.1.0
)

set(XSFDEU_PLUG_BUILD_NAME "eazutil" CACHE STRING "Name used for the built binary.")
set(XSFDEU_PLUG_BUILD_OUTPUT "" CACHE PATH "Optional path to automatically copy the plugin build output. This should be pointing at x64dbg's root directory.")

set(XSFDEU_DOTNET_SDK_PATH "C:/Program Files (x86)/Windows Kits/NETFXSDK" CACHE PATH "dotNet SDK installation path. (NETFXSDK directory)")
set(XSFDEU_DOTNET_SDK_VER "" CACHE STRING "dotNet version SDK to use.")

option(XSFDEU_PLUG_BUILD_OUTPUT_FORCE "Forces XSFDEU_PLUG_BUILD_OUTPUT" OFF)
option(XSFDEU_DEBUG "Debug flag" OFF)

if (NOT EXISTS ${XSFDEU_DOTNET_SDK_PATH})
	message(FATAL_ERROR "The dotNet SDK path (NETFXSDK) was not found. Either dotNet is not installed or is installed somewhere else. Default value is ${XSFDEU_DOTNET_SDK_PATH}. Configure XSFDEU_DOTNET_SDK_PATH to an appropriate path.")
endif()

if (NOT XSFDEU_DOTNET_SDK_VER)
	message("No dotNet SDK version supplied in variable XSFDEU_DOTNET_SDK_VER.\nAvailable dotNet versions:")
	FILE(GLOB dnvers ABSOLUTE "${XSFDEU_DOTNET_SDK_PATH}/*")
	foreach(dnver ${dnvers})
		if (IS_DIRECTORY ${dnver})
			string(REPLACE "${XSFDEU_DOTNET_SDK_PATH}/" "" dnver ${dnver})
			message(${dnver})
		endif()
	endforeach()
	message(FATAL_ERROR "Provide a dotNet version SDK to XSFDEU_DOTNET_SDK_VER variable")
endif()

add_subdirectory("deps/kita")

add_library(
	${PROJECT_NAME}
	SHARED

    "msdnlib.hpp"
	"msdnlib.cpp"	

	"dotnet.hpp"
	"dotnet.cpp"

	"eazlib.hpp"

	"global.hpp"
	"main.cpp"

	"xsfd_utils.hpp"
	"xsfd_utils.cpp"

	"menu.hpp"
	"menu.cpp"

	"callbacks/callbacks.hpp"
	"callbacks/initialize.cpp"	
	"callbacks/uninitialize.cpp"
 )

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(XSFDEU_FILE_EXT "dp64")
	set(XSFDEU_ARCH_TXT "x64")
	set(XSFDEU_ARCH2_TXT "x64")
	target_compile_definitions(${PROJECT_NAME} PRIVATE XSFDEU_ARCH64=1)
elseif (CMAKE_SIZEOF_VOID_P EQUAL 4)
	set(XSFDEU_FILE_EXT "dp32")
	set(XSFDEU_ARCH_TXT "x32")
	set(XSFDEU_ARCH2_TXT "x86")
	target_compile_definitions(${PROJECT_NAME} PRIVATE XSFDEU_ARCH32=1)
else ()
	message(FATAL_ERROR "Unsupported architecture (Invalid CMAKE_SIZEOF_VOID_P)")
endif()

set(XSFDEU_DOTNET_SDK_INCLUDE "${XSFDEU_DOTNET_SDK_PATH}/${XSFDEU_DOTNET_SDK_VER}/Include/um")
set(XSFDEU_DOTNET_SDK_LIB "${XSFDEU_DOTNET_SDK_PATH}/${XSFDEU_DOTNET_SDK_VER}/Lib/um/${XSFDEU_ARCH2_TXT}")

if (XSFDEU_DEBUG)
	target_compile_definitions(
		${PROJECT_NAME}
		PRIVATE
		XSFDEU_DEBUG
	)
endif()

set_target_properties(
	${PROJECT_NAME}
	PROPERTIES
	OUTPUT_NAME "${XSFDEU_PLUG_BUILD_NAME}"
	SUFFIX ".${XSFDEU_FILE_EXT}"
	CXX_STANDARD 20
	CXX_STANDARD_REQUIRED True
)

target_include_directories(
	${PROJECT_NAME}
	PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/deps/pluginsdk"
	"${XSFDEU_DOTNET_SDK_INCLUDE}"
)

target_link_libraries(
	${PROJECT_NAME}
	PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/deps/pluginsdk/${XSFDEU_ARCH_TXT}dbg.lib"
	"${CMAKE_CURRENT_SOURCE_DIR}/deps/pluginsdk/${XSFDEU_ARCH_TXT}bridge.lib"
	"${XSFDEU_DOTNET_SDK_LIB}/mscoree.lib"
	"${XSFDEU_DOTNET_SDK_LIB}/corguids.lib"
	kita
)

# Auto copy build .dp to x64dbg path
if (XSFDEU_PLUG_BUILD_OUTPUT)	
	if (NOT XSFDEU_PLUG_BUILD_OUTPUT_FORCE AND NOT EXISTS "${XSFDEU_PLUG_BUILD_OUTPUT}/x96dbg.exe")
		message(FATAL_ERROR "Invalid XSFDEU_PLUG_BUILD_OUTPUT path! (x64dbg launcher was not found in the directory which is used to determine whether the directory is valid. If this is a mistake you can set the XSFDEU_PLUG_BUILD_OUTPUT_FORCE to true/on)")
	endif()

	set(XSFDEU_PLUG_BUILD_OUT "${XSFDEU_PLUG_BUILD_OUTPUT}/${XSFDEU_ARCH_TXT}/plugins/${XSFDEU_PLUG_BUILD_NAME}.${XSFDEU_FILE_EXT}")
	message("Install plugin on build enabled! (${XSFDEU_PLUG_BUILD_OUT})")

	add_custom_command(
		TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E echo "Copying built binaries to designated plugin folder..."
		COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> "${XSFDEU_PLUG_BUILD_OUT}"
		COMMENT "Copying built binaries to designated plugin folder..."
	)
endif()
