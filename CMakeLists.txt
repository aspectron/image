cmake_minimum_required(VERSION 2.8) 

project(image.jsx)

IF(CMAKE_COMPILER_IS_GNUCXX)
        SET(CMAKE_CXX_FLAGS "-std=c++0x")
endif()


set(JSX_ROOT ~/jsx)
set(EXTERN_ROOT  ${JSX_ROOT}/extern)
set(EXTERN_BUILD ${JSX_ROOT}/extern/build)

set(EXECUTABLE_OUTPUT_PATH ${JSX_ROOT}/bin)
set(LIBRARY_OUTPUT_PATH ${JSX_ROOT}/bin)

# Boost
include_directories(${EXTERN_ROOT}/boost/include)
link_directories(${EXTERN_ROOT}/boost/lib)

# Pion
include_directories(${EXTERN_BUILD}/pion/include)
link_directories(${EXTERN_BUILD}/pion/lib)

# v8
include_directories(${EXTERN_BUILD}/v8/include)
link_directories(${EXTERN_BUILD}/v8/lib)

# V8-juice
include_directories(${EXTERN_BUILD}/v8-juice/include)
link_directories(${EXTERN_BUILD}/v8-juice/lib)

# JSX SDK
include_directories(${JSX_ROOT}/sdk/core/src)
link_directories(${JSX_ROOT}/bin)

# Berkelium
include_directories(extern/berkelium/include)

file(GLOB SRC "src/*.cpp")

set(BOOST_LIBRARIES boost_filesystem boost_system)

add_library(image.jsx SHARED ${SRC})
target_link_libraries(image.jsx core.jsx ${BOOST_LIBRARIES})

