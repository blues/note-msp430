#toolchain cmake file for msp430-gcc/avrdude toolchain

cmake_minimum_required(VERSION 3.20)
# 3.17 - for REQUIRED on find_program
# 3.20 - for cmake_path

# needs the following variables:
# MSP430_MCU : mcu type 
# MSP430_MCU_FREQ : clock frequency (defines F_CPU)


#generic avr flags

set(MSP430_CFLAGS CACHE STRING "MSP430 compilation flags")
set(MSP430_LFLAGS "-Wl,--relax,--gc-sections" CACHE STRING "MSP430 link flags")

#find toolchain programs
set(MSP430_PREFIX "msp430-elf-")
list(APPEND CMAKE_PREFIX_PATH "$ENV{MSP430_GCC_PATH}")
find_program(MSP430-GCC ${MSP430_PREFIX}gcc REQUIRED)
find_program(MSP430-GXX ${MSP430_PREFIX}g++ REQUIRED)
find_program(MSP430-OBJCOPY ${MSP430_PREFIX}objcopy REQUIRED)
find_program(MSP430-SIZE ${MSP430_PREFIX}size REQUIRED)
find_program(MSP430-OBJDUMP ${MSP430_PREFIX}objdump REQUIRED)
find_program(MSPDEBUG mspdebug)

if(NOT MSP430_GCC_PATH)
    cmake_path(SET MSP430_GCC_PATH NORMALIZE "${MSP430-GCC}/../..")
endif(NOT MSP430_GCC_PATH)
message(STATUS "Using directory ${MSP430_GCC_PATH} for msp430-gcc")


#define toolchain
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_C_COMPILER ${MSP430-GCC})
set(CMAKE_CPP_COMPILER ${MSP430-GXX})

#Release by default
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif(NOT CMAKE_BUILD_TYPE)

function(msp430_add_executable_compilation EXECUTABLE)
	
	set(EXECUTABLE_ELF "${EXECUTABLE}.elf")

	# main target for the executable depends of elf
	add_custom_target(${EXECUTABLE} ALL DEPENDS ${EXECUTABLE_ELF})

	# compile and link elf file
	add_executable(${EXECUTABLE_ELF} ${ARGN})

    set_target_properties( PROPERTIES
		LINK_FLAGS "-mmcu=${MSP430_MCU} ${MSP430_LFLAGS}")
    target_include_directories(${EXECUTABLE_ELF} PUBLIC "${MSP430_GCC_PATH}/include")
    target_link_directories(${EXECUTABLE_ELF} PUBLIC "${MSP430_GCC_PATH}/include")
    target_link_options(${EXECUTABLE_ELF} PRIVATE -T${MSP430_MCU}.ld)
	target_compile_options(${EXECUTABLE_ELF} PUBLIC "-Wall")
	# display size info after compilation
	add_custom_command(TARGET ${EXECUTABLE} POST_BUILD
		COMMAND ${MSP430-SIZE} ${EXECUTABLE_ELF})
endfunction(msp430_add_executable_compilation)

function(msp430_add_executable_upload ${EXECUTABLE})
	add_custom_target(upload_${EXECUTABLE} 
		COMMAND ${MSPDEBUG} -q rf2500 "prog ${EXECUTABLE}.elf"
		DEPENDS ${EXECUTABLE})
endfunction(msp430_add_executable_upload)

function(msp430_add_executable EXECUTABLE)
	if(NOT MSP430_MCU)
		message(FATAL_ERROR "MSP430_MCU not defined")
	endif(NOT MSP430_MCU)
	msp430_add_executable_compilation(${EXECUTABLE} ${ARGN})
	msp430_add_executable_upload(${EXECUTABLE})
endfunction(msp430_add_executable)

