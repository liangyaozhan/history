# compiler definition for cross-compile rtk
#

include (CMakeForceCompiler)

macro(CMAKE_FORCE_ASM_COMPILER compiler id)
  set(CMAKE_ASM_COMPILER "${compiler}")
  set(CMAKE_ASM_COMPILER_ID_RUN TRUE)
  set(CMAKE_ASM_COMPILER_ID ${id})
  set(CMAKE_ASM_COMPILER_FORCED TRUE)

  # Set old compiler id variables.
  if("${CMAKE_ASM_COMPILER_ID}" MATCHES "GNU")
    set(CMAKE_COMPILER_IS_GNUCC 1)
  endif()
endmacro()


set(CMAKE_SYSTEM_NAME Generic)
CMAKE_FORCE_C_COMPILER(arm-none-eabi-gcc GNU)
CMAKE_FORCE_CXX_COMPILER (arm-none-eabi-g++ GNU)
CMAKE_FORCE_ASM_COMPILER (arm-none-eabi-g++ GNU)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_AR arm-none-eabi-ar)
set(CMAKE_RANLIB arm-none-eabi-ranlib)

