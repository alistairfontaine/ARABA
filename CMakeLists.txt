cmake_minimum_required(VERSION 3.14)
project(ARABA VERSION 0.9 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Source files
set(ARABA_SOURCES
    src/araba_node.cpp
    src/network.cpp
    src/routing.cpp
    src/persistence.cpp
    src/crypto.cpp
    src/fragment.cpp
    src/config.cpp
)

# Main executable
add_executable(araba_node ${ARABA_SOURCES})

target_include_directories(araba_node PRIVATE include)

target_compile_options(araba_node PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

# Install
install(TARGETS araba_node DESTINATION bin)

# Optional: test target
if(EXISTS "${CMAKE_SOURCE_DIR}/src/test_crypto.cpp")
    add_executable(test_crypto src/test_crypto.cpp src/crypto.cpp)
    target_include_directories(test_crypto PRIVATE include)
endif()
