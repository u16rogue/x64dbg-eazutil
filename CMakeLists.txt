cmake_minimum_required(VERSION 3.18)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)

project(
	x64dbg-eazutil
	LANGUAGES CXX C
	DESCRIPTION "x64dbg Plugin for Eazfuscator"
	VERSION 0.1.0
)

set(XSFDEU_PLUG_BUILD_NAME "eazutil" CACHE STRING "Name used for the built binary.")
set(XSFDEU_PLUG_BUILD_OUTPUT "" CACHE PATH "Optional path to automatically copy the plugin build output. This should be pointing at x64dbg's root directory.")
option(XSFDEU_PLUG_BUILD_OUTPUT_FORCE "Forces XSFDEU_PLUG_BUILD_OUTPUT" OFF)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/deps/dncomlib")

add_library(
	${PROJECT_NAME}
	SHARED
	"${CMAKE_CURRENT_SOURCE_DIR}/global.hpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"

	"${CMAKE_CURRENT_SOURCE_DIR}/xsfd_utils.hpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/xsfd_utils.cpp"
)

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(XSFDEU_FILE_EXT "dp64")
	set(XSFDEU_ARCH_TXT "x64")
else ()
	set(XSFDEU_FILE_EXT "dp32")
	set(XSFDEU_ARCH_TXT "x32")
endif()

set_target_properties(
	${PROJECT_NAME}
	PROPERTIES
	OUTPUT_NAME "${XSFDEU_PLUG_BUILD_NAME}"
	SUFFIX ".${XSFDEU_FILE_EXT}"
)

target_include_directories(
	${PROJECT_NAME}
	PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/deps/pluginsdk"
)

target_link_libraries(
	${PROJECT_NAME}
	PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/deps/pluginsdk/${XSFDEU_ARCH_TXT}dbg.lib"
	"${CMAKE_CURRENT_SOURCE_DIR}/deps/pluginsdk/${XSFDEU_ARCH_TXT}bridge.lib"
	dncomlib
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
		COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> "${XSFDEU_PLUG_BUILD_OUT}"
		COMMENT "Copying built binaries to designated plugin folder..."
	)
endif()
