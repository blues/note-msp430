# use include() to include this file in your project's CMakeLists.txt file.
# The script sets NOTE_C to ./note-c when not already defined.
# Defines `note-c` as a source library. Using a source library avoids problems with link order
# and weak functions.

cmake_minimum_required(VERSION 3.12)
# 3.12 - list transformers - PREPEND

# define NOTE_C if not already defined
if("${NOTE_C}" STREQUAL "")
cmake_path(SET NOTE_C NORMALIZE ${CMAKE_CURRENT_LIST_DIR}/)
endif()

set(NOTE_C_SOURCES_TXT ${NOTE_C}/note-c-sources.txt)

# read the list of sources and prepend with the NOTE_C path
file(STRINGS ${NOTE_C_SOURCES_TXT} NOTE_C_SOURCES)
list(TRANSFORM NOTE_C_SOURCES PREPEND ${NOTE_C})

add_library(note-c OBJECT ${NOTE_C_SOURCES})
target_include_directories(note-c PUBLIC ${NOTE_C})

# add a dependency on note-c.files so that changes to that trigger a rebuild
add_custom_target(note-c-sources-txt DEPENDS ${NOTE_C_SOURCES_TXT})
add_dependencies(note-c note-c-sources-txt)

