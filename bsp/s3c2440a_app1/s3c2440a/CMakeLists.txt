# by liangyaozhan
# cross compile:
# cmake -DCMAKE_TOOLCHAIN_FILE=path/to/file path/to/source
#include (CMakeForceCompiler)
#set(CMAKE_SYSTEM_NAME Generic)
#CMAKE_FORCE_C_COMPILER(arm-none-eabi-gcc GNU)
#CMAKE_FORCE_CXX_COMPILER (arm-none-eabi-g++ GNU)
# 
#set(CMAKE_SYSTEM_PROCESSOR arm)


cmake_minimum_required(VERSION 2.8)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../../../rtk/cmake/arm_none_eabi_gcc.cmake)

PROJECT(mini2440 C ASM)



# compiler definition for cross-compile rtk
#

#include (CMakeForceCompiler)
# 
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
#set(CMAKE_AR arm-none-eabi-ar)
#set(CMAKE_RANLIB arm-none-eabi-ranlib)


ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/../../../rtk rtk_core)


message("rtk_BUILD_INCLUDE=" ${rtk_BUILD_INCLUDE})

set(bsp_SOURCES
  start.S
  sysALib.S
  mmu_setup.c
  s3c2440_clock.c
  s3c2440_lcd.c
  s3c2440_timer.c
  serial.c
  mmu-armarchv4.c
  s3c2440.c
  s3c2440_int.c
#  s3c2440_sd.c
  s3c2440_uart.c
  target_init.c
  main.c
  )

set(bsp_BUILD_INCLUDE
  ${CMAKE_CURRENT_SOURCE_DIR}
  )
  
add_executable(mini2440.elf
  ${bsp_SOURCES}
  )

target_link_libraries(mini2440.elf rtk gcc c)
add_dependencies(mini2440.elf rtk)

target_include_directories(mini2440.elf
  PUBLIC ${bsp_BUILD_INCLUDE}
  )

set(mini2440_LINK_FLAGS
  "--entry _start -nostdlib -nostartfiles -T${CMAKE_CURRENT_SOURCE_DIR}/ldscript.lds"
  )

set_target_properties(mini2440.elf PROPERTIES LINK_FLAGS "${mini2440_LINK_FLAGS}" )

get_target_property(mini2440_incld mini2440 LINK_FLAGS )


message( "C_flags of mini2440 = " ${mini2440_incld})

