cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
set(PICO_BOARD pico)
set(BOARD pico_sdk)
include(pico_sdk_import.cmake)
include(pico_extras_import.cmake)
# include($ENV{PICO_EXTRAS_PATH}/external/pico_extras_import.cmake)

project(pico_examples C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(PICO_EXAMPLES_PATH ${PROJECT_SOURCE_DIR})


# Initialize the SDK
pico_sdk_init()

include(example_auto_set_url.cmake)

# Build project

# include_directories(../engine ../sf2 .. ../pm_embedded)

# add_executable(lptstore lptstore.c lptcomms.c diskfake.c lptprotocol.c)
# pico_set_binary_type(lptstore no_flash)
# target_link_libraries(lptstore pico_stdlib)
# pico_enable_stdio_usb(lptstore 1)
# pico_enable_stdio_uart(lptstore 0)
# pico_add_extra_outputs(lptstore)
# example_auto_set_url(lptstore)
# target_compile_options(lptstore PRIVATE -Wall -Wextra -g  -DPICO_SHARED_IRQ_HANDLER_HIGHEST_ORDER_PRIORITY=PICO_HIGHEST_IRQ_PRIORITY -DPICO_STDIO_USB_DEFAULT_CRLF=1)
# target_include_directories(lptstore PUBLIC ${CMAKE_CURRENT_LIST_DIR})

add_executable(lptstoreusb lptstore.c lptcomms.c lptprotocol.c msc_app.c ../fs/fat32.c)
# pico_set_binary_type(lptstoreusb no_flash)
target_link_libraries(lptstoreusb PUBLIC pico_stdlib tinyusb_host tinyusb_board pico_multicore)
pico_enable_stdio_usb(lptstoreusb 0)
pico_enable_stdio_uart(lptstoreusb 1)
pico_add_extra_outputs(lptstoreusb)
example_auto_set_url(lptstoreusb)
target_link_options(lptstoreusb PRIVATE -Xlinker --print-memory-usage -g)
target_compile_options(lptstoreusb PRIVATE -Wall -Wextra -g  -DPICO_SHARED_IRQ_HANDLER_HIGHEST_ORDER_PRIORITY=PICO_HIGHEST_IRQ_PRIORITY -DTUSB_CFG_DEBUG=2)
target_include_directories(lptstoreusb PUBLIC ${CMAKE_CURRENT_LIST_DIR} ../fs)
