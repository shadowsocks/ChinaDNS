cmake_minimum_required (VERSION 2.8.0)

project ( chinadns-openwrt )

include_directories (
		${CMAKE_CURRENT_SOURCE_DIR}/src
	)

set (HDR
	)

set (SRC_chinadns
		${CMAKE_CURRENT_SOURCE_DIR}/src/chinadns.c
	)


add_executable( chinadns ${SRC_chinadns} ${HDR} )
target_link_libraries( chinadns resolv)
