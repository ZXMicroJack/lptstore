cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)
include(pico_extras_import.cmake)
# include($ENV{PICO_EXTRAS_PATH}/external/pico_extras_import.cmake)

project(pico_examples C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_8BA -DSAMPLE_RATE=22050 -DSAMPLE_LEN=512 -DPICO_AUDIO_I2S_DATA_PIN=4 -DPICO_AUDIO_I2S_CLOCK_PIN_BASE=2 -DVE_STEPS=64 -DLFO_STEPS=64 -DNONAME")

set(PICO_EXAMPLES_PATH ${PROJECT_SOURCE_DIR})


# Initialize the SDK
pico_sdk_init()

include(example_auto_set_url.cmake)

# Build project

# include_directories(../engine ../sf2 .. ../pm_embedded)

# sine wave over i2s test program
add_executable(lptstore lptstore.c lptcomms.c diskfake.c lptprotocol.c)
pico_set_binary_type(lptstore no_flash)
target_link_libraries(lptstore pico_stdlib)
pico_enable_stdio_usb(lptstore 1)
pico_enable_stdio_uart(lptstore 0)
pico_add_extra_outputs(lptstore)
example_auto_set_url(lptstore)

