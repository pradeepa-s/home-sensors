# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NEVER EDIT THE NEXT LINES for Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.0.0)
set(toolchainVersion 13_2_Rel1)
set(picotoolVersion 2.0.0)
include(${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(pico-moisture-v1 C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

set(IP "192.168.20.42")
set(PORT 50000)
set(SSID "P&C_Printer")
set(PWORD "")

if(IP AND PORT AND SSID AND PWORD)
  message("IP is ${IP}")
  message("PORT is ${PORT}")
  message("SSID is ${SSID}")
  message("PWORD is ${PWORD}")
else()
  message(FATAL_ERROR "Define network configuations.")
endif()

# Generate a header with all build args for wifi-name, wifi-password, ip, port
configure_file(${CMAKE_CURRENT_LIST_DIR}/network_conf.h.in ${CMAKE_SOURCE_DIR}/generated/network_conf.h)

# Add executable. Default name is the project name, version 0.1
add_executable(pico-moisture-v1 pico-moisture-v1.c network.c)

# add_dependencies(${CMAKE_PROJECT_NAME} NetworkHeader)

pico_set_program_name(pico-moisture-v1 "pico-moisture-v1")
pico_set_program_version(pico-moisture-v1 "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(pico-moisture-v1 1)
pico_enable_stdio_usb(pico-moisture-v1 0)

# Add the standard library to the build
target_link_libraries(pico-moisture-v1
        pico_stdlib
        pico_cyw43_arch_lwip_poll)

# Add the standard include files to the build
target_include_directories(pico-moisture-v1 PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts or any other standard includes, if required
  ${CMAKE_CURRENT_LIST_DIR}/generated
)

pico_add_extra_outputs(pico-moisture-v1)

