cmake_minimum_required(VERSION 3.0.2)
project (bmonmate)
set(SDK_NAME SDK)
set(CMAKE_MODULE_PATH ${PROJECT_ROOT_PATH}/${SDK_NAME}/cmake)

SET(MY_CXX_FLAGS "-Wno-error=deprecated-declarations -Wno-error=return-type ")
#---------------------------------------------------------------

get_filename_component(SDK_PATH ${CMAKE_MODULE_PATH} DIRECTORY)

if (NOT SDK_PATH)
    message(FATAL_ERROR "ROBOT_SDK_PATH must be set")
endif ()

set(BENDER_DIR ${SDK_PATH})
set(3RD_PARTY_DIR ${BENDER_DIR}/3rdParty)

if(MSVC)
    include(${CMAKE_MODULE_PATH}/MSVC_Env.cmake)
elseif(UNIX)
    include(${CMAKE_MODULE_PATH}/Linux_Env.cmake)
endif(MSVC)

set(PLATFORM_INCLUDE_DIR ${BENDER_DIR}/Include)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

link_directories(${Boost_LIBRARY_DIRS})
#link_directories(${BENDER_DIR}/Bin/RHEL7)
add_subdirectory(benderjs)

if(CMAKE_BUILD_TYPE)
    string( TOUPPER ${CMAKE_BUILD_TYPE} OUTPUTCONFIG )
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME}_${OUTPUTCONFIG})
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME}_${OUTPUTCONFIG})
    set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME}_${OUTPUTCONFIG})
else(CMAKE_BUILD_TYPE)
    foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
        string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
        set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/${PROJECT_NAME}_${OUTPUTCONFIG} )
        set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/${PROJECT_NAME}_${OUTPUTCONFIG} )
        set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/${PROJECT_NAME}_${OUTPUTCONFIG} )
    endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )
endif(CMAKE_BUILD_TYPE)

#---------------------------------------------------------------
# Add my flags
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MY_CXX_FLAGS}")

# Find necessary packages
find_package(Boost 1.71.0 REQUIRED COMPONENTS system)
#link_directories(${SDK_PATH}/Bin/RHEL7)
link_directories(/home/user/bender-docker/build/Bender2_DEBUG)
set(BENDER_LIBRARIES Core)
#---------------------------------------------------------------

file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

#---------------------------------------------------------------
set(TARGET_NAME bmonmate)
#---------------------------------------------------------------

add_library(${TARGET_NAME} SHARED ${SOURCES})
target_link_libraries(${TARGET_NAME} benderjs ${Boost_LIBRARIES} ${BENDER_LIBRARIES})

#if (MSVC)
#    target_link_libraries(${TARGET_NAME} ${Boost_LIBRARIES} Core)
#else (MSVC)
#    target_link_libraries(${TARGET_NAME} ${Boost_LIBRARIES} ${BENDER_LIBRARIES})
#endif (MSVC)

#set_target_properties(${TARGET_NAME} PROPERTIES FOLDER Robots)
target_compile_definitions(${TARGET_NAME} PRIVATE BOOST_USER_CONFIG=<boost/config/bender_config.hpp> BENDER_USES_STDCXX11)
target_include_directories(${TARGET_NAME} SYSTEM PRIVATE ${Boost_INCLUDE_DIRS})
target_include_directories(${TARGET_NAME} PRIVATE ${OPENSSL_INCLUDE_DIR} ${PLATFORM_INCLUDE_DIR})
#---------------------------------------------------------------

install(TARGETS bmonmate DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/../)
