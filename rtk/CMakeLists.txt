# CMakeLists.txt for rtk, by liangyaozhan
# cross compile:
# cmake -DCMAKE_TOOLCHAIN_FILE=path/to/file path/to/source
#include (CMakeForceCompiler)
#set(CMAKE_SYSTEM_NAME Generic)
#CMAKE_FORCE_C_COMPILER(arm-none-eabi-gcc GNU)
#CMAKE_FORCE_CXX_COMPILER (arm-none-eabi-g++ GNU)
# 
#set(CMAKE_SYSTEM_PROCESSOR arm)


cmake_minimum_required(VERSION 2.8)

PROJECT(rtk C CXX ASM)

#####################################################################
#include (CMakeForceCompiler)
#macro(CMAKE_FORCE_ASM_COMPILER compiler id)
#  set(CMAKE_ASM_COMPILER "${compiler}")
#  set(CMAKE_ASM_COMPILER_ID_RUN TRUE)
#  set(CMAKE_ASM_COMPILER_ID ${id})
#  set(CMAKE_ASM_COMPILER_FORCED TRUE)
# 
#  # Set old compiler id variables.
#  if("${CMAKE_ASM_COMPILER_ID}" MATCHES "GNU")
#    set(CMAKE_COMPILER_IS_GNUCC 1)
#  endif()
#endmacro()
# 
# 
#set(CMAKE_SYSTEM_NAME Generic)
#CMAKE_FORCE_C_COMPILER(arm-none-eabi-gcc GNU)
#CMAKE_FORCE_CXX_COMPILER (arm-none-eabi-g++ GNU)
#CMAKE_FORCE_ASM_COMPILER (arm-none-eabi-g++ GNU)
#set(CMAKE_SYSTEM_PROCESSOR arm)
# 
#set(CMAKE_AR arm-none-eabi-ar)
#set(CMAKE_RANLIB arm-none-eabi-ranlib)
#####################################################################

if(NOT CMAKE_TOOLCHAIN_FILE)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/arm_none_eabi_gcc.cmake)
message("using default CMAKE_TOOLCHAIN_FILE=" ${CMAKE_TOOLCHAIN_FILE} )
endif()

if(NOT CMAKE_AR)
set(CMAKE_AR arm-none-eabi-ar)
message("using default CMAKE_AR=" ${CMAKE_AR})
endif()

if(NOT CMAKE_RANLIB)
set(CMAKE_RANLIB arm-none-eabi-ranlib)
message("using default CMAKE_RANLIB=" ${CMAKE_RANLIB})
endif()

# rtk core sources
set(rtk_SOURCES core/rtk.c)

# rtk arch sources
set(rtk_ARCH_SOURCES
  arch/arm/armv4_context_gcc.S
  arch/arm/armv4_exc_gcc.S
  arch/arm/armv4_context.c
  arch/arm/armv4_exc.c
  )

set(rtk_UTIL_SOURCES
  util/creating_libs.c
  util/kprintf.c
  util/minilibc.c
  util/slab.c
  )

set(rtk_DEFINITION
  -DALIGN_SIZE=8
  -DMEM_STATS
  )

set(rtk_COMPILER_OPTIONS
  -Wall
  -fmessage-length=0
  -fno-short-enums
  )


# rtk build include
set(rtk_BUILD_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/core )

# librtk.a
add_library(rtk ${rtk_SOURCES}
				${rtk_ARCH_SOURCES}
				${rtk_UTIL_SOURCES}
				)

# add librtk.a include
target_include_directories(rtk PUBLIC ${rtk_BUILD_INCLUDE} )

# librtk.a definition
target_compile_definitions(rtk
  PUBLIC ${rtk_DEFINITION}
  )

# librtk.a compiler options
target_compile_options(rtk 
  PUBLIC ${rtk_COMPILER_OPTIONS}
  )


