cmake_minimum_required(VERSION 3.0)
project(led-strip)

set(kerneldir "" CACHE STRING "Path to the kernel build directory")

if("${kerneldir}" STREQUAL "")
  execute_process(COMMAND uname -r OUTPUT_VARIABLE uname_r
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
  set(kerneldir "/lib/modules/${uname_r}/build")
endif()

find_file(kernel_makefile NAMES Makefile
                          PATHS ${kerneldir} NO_DEFAULT_PATH)

if(NOT kernel_makefile)
  message(FATAL_ERROR "There is no Makefile in kerneldir!")
endif()

list(APPEND module_sources
  led_control.c
  led_strip.c
  led_control.h
  led_strip.h
)
list(APPEND module_headers
)
string(REPLACE ";" " " module_sources_string "${module_sources}")
configure_file(Kbuild.in Kbuild @ONLY)
foreach(src ${module_sources})
  configure_file(${src} ${src} COPYONLY)
endforeach()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(module_cmd ${CMAKE_MAKE_PROGRAM} -C ${kerneldir} M=${CMAKE_CURRENT_BINARY_DIR})
add_custom_command(OUTPUT led-strip.ko
  COMMAND ${module_cmd} modules
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS ${module_sources} ${module_headers} ${CMAKE_CURRENT_BINARY_DIR}/Kbuild
  VERBATIM)
add_custom_target(module DEPENDS led-strip.ko)
add_custom_target(module-clean COMMAND ${module_cmd} clean)
