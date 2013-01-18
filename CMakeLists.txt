cmake_minimum_required(VERSION 2.8) 

project(image.jsx)

set(JSX_ROOT ~/jsx)

include(${JSX_ROOT}/CMakeLists.txt.global)

file(GLOB SRC "src/*.cpp")

set(BOOST_LIBRARIES boost_filesystem boost_system)

add_library(image.jsx SHARED ${SRC})
target_link_libraries(image.jsx core.jsx ${BOOST_LIBRARIES})

