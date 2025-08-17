cmake_minimum_required(VERSION 3.15.0)
project(installer VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(USE_STATIC_LINKING "Use static linking" OFF)

if(CMAKE_BUILD_TYPE STREQUAL "Release" OR FORCE_STATIC_LINKING)
    set(USE_STATIC_LINKING ON)
    message(STATUS "Static linking enabled")
endif()

if(USE_STATIC_LINKING)
    set(VCPKG_TARGET_TRIPLET "x64-linux-static")
    message(STATUS "Using vcpkg triplet: x64-linux-static")
else()
    set(VCPKG_TARGET_TRIPLET "x64-linux")
    message(STATUS "Using vcpkg triplet: x64-linux")
endif()

if(USE_STATIC_LINKING)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    set(BUILD_SHARED_LIBS OFF)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
    
    # set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
endif()


find_package(CURL CONFIG REQUIRED)
find_package(libzip CONFIG REQUIRED)
find_package(jsoncpp CONFIG REQUIRED)

file(GLOB_RECURSE PROJ_SRC src/*.c*)

add_executable(installer ${PROJ_SRC})

target_include_directories(installer PRIVATE 
    src/include
)


target_link_libraries(installer PRIVATE
    CURL::libcurl
    libzip::zip
    JsonCpp::JsonCpp   
)


if(USE_STATIC_LINKING)
    
    find_package(Threads REQUIRED)
    target_link_libraries(installer PRIVATE 
        Threads::Threads
        ${CMAKE_DL_LIBS}
    )
endif()
