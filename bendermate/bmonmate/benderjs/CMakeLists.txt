# CMakeLists files in this project can
# refer to the root source directory of the project as ${HELLO_SOURCE_DIR} and
# to the root binary directory of the project as ${HELLO_BINARY_DIR}.
cmake_minimum_required (VERSION 3.10.2)
project (benderjs C)
set(CMAKE_C_FLAGS "-g -Wall -MMD -Wno-array-bounds -D_GNU_SOURCE -DCONFIG_VERSION=2019-07-01 -w -fPIC")
file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
set(TARGET_NAME benderjs)
add_library (${TARGET_NAME} STATIC qjs.c quickjs.c cutils.c quickjs-libc.c repl.c libunicode.c libregexp.c bmonmate.c)
#target_link_libraries(${TARGET_NAME} bmonmate )
#install(TARGETS benderjs DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/../..)

