cmake_minimum_required(VERSION 4.3.0)
project(DiskDetector)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

file(GLOB_RECURSE source CONFIGURE_DEPENDS "./src/*.hpp"
					"./src/*.cpp" )

file(GLOB_RECURSE modules CONFIGURE_DEPENDS "./modules/*.cppm")
add_executable(main ${source})
target_sources(main PUBLIC FILE_SET CXX_MODULES FILES ${modules})
target_link_libraries(main PUBLIC blkid econf)
target_link_libraries(main PUBLIC -static)


file(GLOB_RECURSE keycheck_source CONFIGURE_DEPENDS "./keycheck/src/*.hpp"
					"./keycheck/src/*.cpp" )
add_executable(keycheck ${keycheck_source})

file(GLOB_RECURSE onmount_source CONFIGURE_DEPENDS "./onmount/src/*.hpp"
					"./onmount/src/*.cpp" )
add_executable(onmount ${onmount_source})
