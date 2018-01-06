cmake_minimum_required (VERSION 3.6)
project (kvak)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_EXPORT_COMPILE_COMMANDS ON)
set (CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_SOURCE_DIR}/cmake/")

find_package (CapnProto REQUIRED)

option (WITH_OPTIMIZATION "Enable aggressive compiler optimizations" ON)
option (WITH_FAST_MATH "Enable non-IEEE compliant math optimizations" ON)
option (WITH_NATIVE "Enable native compiler optimizations (may make the binary
non-portable" ON)
option (WITH_ASAN "Enable ASan" OFF)
option (WITH_MSAN "Enable MSan" OFF)
option (WITH_UBSAN "Enable UBSan" OFF)
option (WITH_OPENMP "Enable OpenMP" ON)

if (WITH_OPTIMIZATION)
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -flto")
	set (CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -flto")
endif ()
if (WITH_FAST_MATH)
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffast-math")
endif ()
if (WITH_NATIVE)
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
endif ()
if (WITH_ASAN)
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -fsanitize=address")
	set (CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fno-omit-frame-pointer -fsanitize=address")
endif ()
if (WITH_MSAN)
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=memory")
endif ()
if (WITH_UBSAN)
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
endif ()
if (WITH_OPENMP)
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")
endif ()

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter")
set (CMAKE_CXX_FLAGS "${CMAKE_LINKER_FLAGS} -lpthread")

set (CAPNPC_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
include_directories (${CAPNP_INCLUDE_DIRS} ${CAPNPC_OUTPUT_DIR})
add_definitions (${CAPNP_DEFINITIONS})
capnp_generate_cpp (CAPNP_SRCS CAPNP_HDRS schema.capnp)

set (INTERPOL_TAPS_FILE_PATH "${CMAKE_CURRENT_BINARY_DIR}/interpol_taps.hpp")
add_custom_command(
	OUTPUT interpol_taps.hpp
	COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/interpol.py -n 8 -s 128 -f c++ -o "${INTERPOL_TAPS_FILE_PATH}"
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
	COMMENT "Generating interpolation filter taps"
)

add_executable (kvak main.cpp costas.cpp demodulator.cpp interpolator.cpp
	gardner.cpp utils.cpp server.cpp log.cpp agc.cpp
	${CAPNP_SRCS} ${CAPNP_HDRS} ${INTERPOL_TAPS_FILE_PATH})
target_link_libraries (kvak stdc++fs ${CAPNP_LIBRARIES})
target_include_directories (kvak PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR})

add_executable (resample resample.cpp interpolator.cpp ${INTERPOL_TAPS_FILE_PATH})
target_include_directories (resample PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR})


install (TARGETS kvak
	RUNTIME DESTINATION bin COMPONENT bin)
install (PROGRAMS control/control.py RENAME kvakctl
	DESTINATION bin COMPONENT bin)
install (FILES schema.capnp
	DESTINATION share/kvak)
