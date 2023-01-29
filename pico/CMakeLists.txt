cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)
include(pico_extras_import.cmake)
# include($ENV{PICO_EXTRAS_PATH}/external/pico_extras_import.cmake)

project(pico_examples C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_8BA -DSAMPLE_RATE=22050 -DSAMPLE_LEN=512 -DPICO_AUDIO_I2S_DATA_PIN=4 -DPICO_AUDIO_I2S_CLOCK_PIN_BASE=2 -DVE_STEPS=64 -DLFO_STEPS=64 -DNONAME")

set(PICO_EXAMPLES_PATH ${PROJECT_SOURCE_DIR})

# Initialize the SDK
pico_sdk_init()

include(example_auto_set_url.cmake)

# Build project

include_directories(../engine ../sf2 .. ../pm_embedded)

# sine wave over i2s test program
add_executable(sine_wave sine_wave.c)
target_link_libraries(sine_wave pico_stdlib hardware_spi pico_audio_i2s)
pico_enable_stdio_usb(sine_wave 1)
pico_enable_stdio_uart(sine_wave 0)
pico_add_extra_outputs(sine_wave)
example_auto_set_url(sine_wave)

# wtsynth
add_executable(wtsynth
    playmidpico.c ../engine/wtm.c ../engine/filter.c ../engine/luts.c ../engine/wtmidi.c ../engine/fader.c ../pm_embedded/wtpatch_stubs.c ../pm_embedded/data.c)
target_link_libraries(wtsynth pico_stdlib hardware_spi pico_audio_i2s )
add_compile_options(-O2)
pico_enable_stdio_usb(wtsynth 1)
pico_enable_stdio_uart(wtsynth 0)
pico_add_extra_outputs(wtsynth)
example_auto_set_url(wtsynth)

# midi module
add_executable(picosynth
    picosynth.c ../engine/wtm.c ../engine/filter.c ../engine/luts.c ../engine/wtmidi.c ../engine/fader.c ../pm_embedded/wtpatch_stubs.c ../pm_embedded/data.c)
target_link_libraries(picosynth pico_stdlib hardware_spi pico_audio_i2s pico_multicore)
add_compile_options(-O2)
pico_enable_stdio_usb(picosynth 1)
pico_enable_stdio_uart(picosynth 0)
pico_add_extra_outputs(picosynth)
example_auto_set_url(picosynth)
target_compile_options(picosynth PUBLIC "-DZXUNO")

# midi module
add_executable(picosynthMT32pi
    picosynth.c ../engine/wtm.c ../engine/filter.c ../engine/wtmidi.c ../engine/fader.c ../pm_flash/wtpatch_stubs.c ../pm_flash/data.c)
target_link_libraries(picosynthMT32pi pico_stdlib hardware_spi pico_audio_i2s pico_multicore)
pico_enable_stdio_usb(picosynthMT32pi 1)
add_compile_options(-O2)
pico_enable_stdio_uart(picosynthMT32pi 0)
pico_add_extra_outputs(picosynthMT32pi)
example_auto_set_url(picosynthMT32pi)

# update module
add_executable(update update.c)
target_link_libraries(update pico_stdlib)
pico_set_binary_type(update no_flash)
pico_enable_stdio_usb(update 1)
pico_enable_stdio_uart(update 0)
pico_add_extra_outputs(update)
example_auto_set_url(update)

# midi module
add_executable(picosynthEO
    picosynth.c ../engine/wtm.c ../engine/filter.c ../engine/wtmidi.c ../engine/fader.c ../pm_flash/wtpatch_stubs.c ../pm_flash/data.c)
target_link_libraries(picosynthEO pico_stdlib hardware_spi pico_audio_i2s pico_multicore)
add_compile_options(-O2)
pico_enable_stdio_usb(picosynthEO 1)
pico_enable_stdio_uart(picosynthEO 0)
pico_add_extra_outputs(picosynthEO)
example_auto_set_url(picosynthEO)
target_compile_options(picosynthEO PUBLIC "-DZXUNO")

# midi module
add_executable(picosynthEORAM
    picosynth.c ../engine/wtm.c ../engine/filter.c ../engine/wtmidi.c ../engine/fader.c ../pm_flash/wtpatch_stubs.c ../pm_flash/data.c)
target_link_libraries(picosynthEORAM pico_stdlib hardware_spi pico_audio_i2s pico_multicore)
add_compile_options(-O2)
pico_enable_stdio_usb(picosynthEORAM 1)
pico_enable_stdio_uart(picosynthEORAM 0)
pico_add_extra_outputs(picosynthEORAM)
example_auto_set_url(picosynthEORAM)
target_compile_options(picosynthEORAM PUBLIC "-DZXUNO -DRAM_BUILD")
pico_set_binary_type(picosynthEORAM no_flash)

