cmake_minimum_required(VERSION 2.6)

project(MaRISC C CXX)

# vsade okrem windozov. TODO
#if (CMAKE_BUILD_TYPE STREQUAL "Debug")
#	set(wxWidgets_CONFIG_OPTIONS --debug=yes)
#else()
#	set(wxWidgets_CONFIG_OPTIONS --debug=no)
#endif()

include_directories(${CMAKE_SOURCE_DIR}/include)

if(APPLE)
	include_directories(${PROJECT_SOURCE_DIR}/include/osx)
endif()

add_subdirectory(src)
add_subdirectory(include)
add_subdirectory(test)

SET(plugin_dest_dir bin)
SET(qtconf_dest_dir bin)
SET(DIRS ${wxWidgets_LIBRARY_DIRS})

