cmake_minimum_required (VERSION 2.6.0)

project ( chinadns-openwrt )

OPTION(WITH_DEBUG "debugging support" OFF)
OPTION(WITH_UCI "OpenWRT UCI support" ON)

include_directories (
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_BINARY_DIR}
    )

configure_file (
    "${CMAKE_CURRENT_SOURCE_DIR}/src/config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/config.h"
    )

set (HDR
    )

set (SRC_chinadns
        ${CMAKE_CURRENT_SOURCE_DIR}/src/chinadns.c
    )

add_executable(chinadns ${SRC_chinadns} ${HDR})

target_link_libraries( chinadns resolv)



set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
    "${CMAKE_CURRENT_SOURCE_DIR}/modules")

if(WITH_UCI)
    find_package( UCI REQUIRED )
    include_directories( ${UCI_INCLUDE_DIR} )
    target_link_libraries( chinadns ${UCI_LIBRARIES})
endif()

