cmake_minimum_required(VERSION 3.12)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(PICO_BOARD pico_w)
include(pico_sdk_import.cmake)
project(Pico_ePaper_Code)
pico_sdk_init()

# add compiled files subdirectory
add_subdirectory(lib/Config)
add_subdirectory(lib/LCD)
add_subdirectory(lib/GUI)

include_directories(./lib/Config)
include_directories(./lib/GUI)
include_directories(./lib/LCD)


add_executable(main 
main.c
)

# enable usb output, disable uart output
pico_enable_stdio_usb(main 1)
pico_enable_stdio_uart(main 0)

pico_add_extra_outputs(main)
target_include_directories(main PRIVATE ${CMAKE_CURRENT_LIST_DIR} )
target_link_libraries(main LCD GUI Config pico_stdlib hardware_spi pico_stdlib pico_stdlib pico_cyw43_arch_lwip_threadsafe_background pico_lwip_http hardware_rtc hardware_flash)
