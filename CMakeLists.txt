cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME netxduo_h723)

# Include toolchain file
include("cmake/gcc-arm-none-eabi.cmake")

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

file(GLOB_RECURSE SrcThreadX 
    "${PROJECT_SOURCE_DIR}/ThirdPartys/ThreadX/common/src/*.c" 
    "${PROJECT_SOURCE_DIR}/ThirdPartys/ThreadX/ports/cortex_m7/gnu/src/*.S"
    "${PROJECT_SOURCE_DIR}/ThirdPartys/ThreadX/ports/cortex_m7/gnu/example_build/tx_initialize_low_level.S"
    )

file(GLOB_RECURSE SrcNetX
    "${PROJECT_SOURCE_DIR}/ThirdPartys/NetXDuo/common/src/*.c" 
    "${PROJECT_SOURCE_DIR}/ThirdPartys/NetXDuo/ports/*.c"
    "${PROJECT_SOURCE_DIR}/ThirdPartys/NetXDuo/addons/dns/*.c"
    "${PROJECT_SOURCE_DIR}/ThirdPartys/NetXDuo/addons/sntp/*.c"
    "${PROJECT_SOURCE_DIR}/ThirdPartys/NetXDuo/addons/telnet/*.c"
    )

file(GLOB_RECURSE SrcApp
    "${PROJECT_SOURCE_DIR}/Threads/src/*.c"
    )


# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    ${SrcThreadX}
    ${SrcNetX}
    ${SrcApp}
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    ${PROJECT_SOURCE_DIR}/ThirdPartys/ThreadX/common/inc
    ${PROJECT_SOURCE_DIR}/ThirdPartys/ThreadX/ports/cortex_m7/gnu/inc
    ${PROJECT_SOURCE_DIR}/ThirdPartys/NetXDuo/common/inc
    ${PROJECT_SOURCE_DIR}/ThirdPartys/NetXDuo/ports/cortex_m7/gnu/inc
    ${PROJECT_SOURCE_DIR}/ThirdPartys/NetXDuo/ports/cortex_m7
    ${PROJECT_SOURCE_DIR}/ThirdPartys/NetXDuo/nx_secure/inc
    ${PROJECT_SOURCE_DIR}/ThirdPartys/NetXDuo/nx_secure/ports
    ${PROJECT_SOURCE_DIR}/ThirdPartys/NetXDuo/crypto_libraries/ports/cortex-m7/gnu/inc
    ${PROJECT_SOURCE_DIR}/ThirdPartys/NetXDuo/crypto_libraries/inc
    ${PROJECT_SOURCE_DIR}/ThirdPartys/NetXDuo/user
    ${PROJECT_SOURCE_DIR}/Threads/inc
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
    DEBUG
    ARM_MATH_CM7
    USE_HAL_DRIVER
    STM32H723xx
    TX_ENABLE_FPU_SUPPORT
)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)

target_compile_options(
    ${CMAKE_PROJECT_NAME} PRIVATE
    "-Wno-unused-variable"
    "-Wno-unused-function"
    "-Wno-unused-parameter"
    "-Wno-sign-compare"
    "-mcpu=cortex-m7"
    "-mfpu=fpv5-d16" 
    "-mfloat-abi=hard"
    #"-Wl$<COMMA>-u_printf_float"
)


target_link_options(
    ${CMAKE_PROJECT_NAME} PRIVATE
    "-mcpu=cortex-m7"
    "-mfpu=fpv5-d16"
    "-mfloat-abi=hard"
    #-T
    #"${PROJECT_SOURCE_DIR}/STM32F767IGTx_FLASH.ld"
    "-Wl$<COMMA>-u_printf_float"
)

#set_target_properties(MyApp PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/custom_bin)
add_custom_command(
    TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex
    $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_PROJECT_NAME}.hex
)

add_custom_command(
    TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary
    $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_PROJECT_NAME}.bin
)


