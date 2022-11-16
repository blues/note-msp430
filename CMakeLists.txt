cmake_minimum_required(VERSION 2.8)

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/generic-msp430-gcc.cmake")

# Name your project
project(EXAMPLE)

#device dependent config, here for launchpad
set(MSP430_MCU msp430fr2355)
set(MSP430_MCU_FREQ 24000000UL)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
# set(CMAKE_C_EXTENSIONS ON)

add_compile_options(-Werror -ffunction-sections -fdata-sections -mmcu=${MSP430_MCU} -DF_CPU=${MSP430_MCU_FREQ})


# note-c
set(NOTE_C ${CMAKE_CURRENT_LIST_DIR}/note-c)
add_library(note-c OBJECT
    ${NOTE_C}/n_atof.c
    ${NOTE_C}/n_b64.c
    ${NOTE_C}/n_cjson.c
    ${NOTE_C}/n_cjson_helpers.c
    ${NOTE_C}/n_const.c
    ${NOTE_C}/n_ftoa.c
    ${NOTE_C}/n_helpers.c
    ${NOTE_C}/n_hooks.c
    ${NOTE_C}/n_i2c.c
#    ${NOTE_C}/n_md5.c
    ${NOTE_C}/n_request.c
    ${NOTE_C}/n_serial.c
    ${NOTE_C}/n_str.c
#    ${NOTE_C}/n_ua.c
)
target_include_directories(note-c
    PUBLIC ${NOTE_C}
)

# driverlib
cmake_path(SET DRIVERLIB NORMALIZE "${CMAKE_CURRENT_LIST_DIR}/driverlib/MSP430FR2xx_4xx")
add_library(driverlib OBJECT
    ${DRIVERLIB}/cs.c
    ${DRIVERLIB}/eusci_b_i2c.c
    ${DRIVERLIB}/eusci_a_uart.c
    ${DRIVERLIB}/framctl.c
    ${DRIVERLIB}/gpio.c
    ${DRIVERLIB}/pmm.c
    ${DRIVERLIB}/timer_b.c
    ${DRIVERLIB}/wdt_a.c
)

target_include_directories(driverlib PUBLIC "${DRIVERLIB}" "${MSP430_GCC_PATH}/include")

function(msp430_add_executable_and_dependencies EXECUTABLE)
	set(EXECUTABLE_ELF "${EXECUTABLE}.elf")
    msp430_add_executable(${EXECUTABLE} main.c ${ARGN})
    # include the source root for main.h
    target_include_directories(${EXECUTABLE_ELF} PUBLIC "./")
    target_link_libraries(${EXECUTABLE_ELF} note-c driverlib mul_f5)
endfunction(msp430_add_executable_and_dependencies)

# example with i2c
msp430_add_executable_and_dependencies(example_i2c example.c)
target_compile_definitions(example_i2c.elf PUBLIC NOTECARD_USE_I2C=true DISABLE_NOTE_C_LIBRARY=false)

# example with serial
msp430_add_executable_and_dependencies(example_serial example.c)
target_compile_definitions(example_serial.elf PUBLIC NOTECARD_USE_I2C=false DISABLE_NOTE_C_LIBRARY=false)


# example min
msp430_add_executable_and_dependencies(example_min example_min.c)
target_compile_definitions(example_min.elf PUBLIC NOTECARD_USE_I2C=false DISABLE_NOTE_C_LIBRARY=true)