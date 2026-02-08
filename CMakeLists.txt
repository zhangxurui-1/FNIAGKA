cmake_minimum_required(VERSION 3.16)
project(FNIAGKA LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# path
set(MIRACL_ROOT "${CMAKE_SOURCE_DIR}/deps/miracl")
set(MIRACL_BUILD "${CMAKE_SOURCE_DIR}/build/miracl")
set(LOCAL_MIRACL_INCLUDE "${CMAKE_SOURCE_DIR}/include/miracl")
set(MIRACL_CORE_LIB "${MIRACL_BUILD}/libMIRACL-core.a")
set(MIRACL_CPP_LIB "${MIRACL_BUILD}/libMIRACL-cpp.a")

# add lib
add_library(MIRACL-core STATIC IMPORTED GLOBAL)
set_target_properties(MIRACL-core PROPERTIES
    IMPORTED_LOCATION ${MIRACL_CORE_LIB}
    INTERFACE_INCLUDE_DIRECTORIES ${LOCAL_MIRACL_INCLUDE}
)

add_library(MIRACL-cpp STATIC IMPORTED GLOBAL)
set_target_properties(MIRACL-cpp PROPERTIES
    IMPORTED_LOCATION ${MIRACL_CPP_LIB}
    INTERFACE_INCLUDE_DIRECTORIES ${LOCAL_MIRACL_INCLUDE}
)

add_executable(main
    src/main.cc
    src/proto.cc
)

target_include_directories(main PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
    "${LOCAL_MIRACL_INCLUDE}"
    "${MIRACL_ROOT}/source"
    "${MIRACL_ROOT}/source/curve/pairing"
)

target_link_libraries(main PRIVATE
    MIRACL-core
    MIRACL-cpp
)