cmake_minimum_required(VERSION 3.16)

project(aislib
    VERSION 0.1.0
    DESCRIPTION "NMEA 0183 v4.1 AIS message parsing and generation library"
    LANGUAGES C CXX
)

# Enforce C++17 across all targets
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for tooling (clangd, etc.)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build type default
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Compiler warning flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wconversion
        -Wsign-conversion
        -Wnull-dereference
        -Wdouble-promotion
        -Wformat=2
    )
elseif(MSVC)
    add_compile_options(/W4 /WX)
endif()

# Coverage support (GCC/Clang only, enabled via -DAISLIB_COVERAGE=ON)
option(AISLIB_COVERAGE "Enable gcov/lcov coverage instrumentation" OFF)
if(AISLIB_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(--coverage -O0 -g)
    add_link_options(--coverage)
endif()

# Option to build tests (default ON)
option(AISLIB_BUILD_TESTS "Build unit tests" ON)

# Option to build as shared library
option(AISLIB_BUILD_SHARED "Build aislib as a shared library" OFF)

# -----------------------------------------------------------------------------
# Library target
# -----------------------------------------------------------------------------
set(AISLIB_SOURCES
    src/error.cpp
    src/bitfield.cpp
    src/payload.cpp
    src/sentence.cpp
    src/message.cpp
)

if(AISLIB_BUILD_SHARED)
    add_library(aislib SHARED ${AISLIB_SOURCES})
else()
    add_library(aislib STATIC ${AISLIB_SOURCES})
endif()

add_library(aislib::aislib ALIAS aislib)

target_include_directories(aislib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(aislib PROPERTIES
    VERSION   ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    POSITION_INDEPENDENT_CODE ON
)

# -----------------------------------------------------------------------------
# Tests
# -----------------------------------------------------------------------------
if(AISLIB_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# -----------------------------------------------------------------------------
# Installation
# -----------------------------------------------------------------------------
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS aislib
    EXPORT  aislibTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/aislib
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT aislibTargets
    FILE      aislibTargets.cmake
    NAMESPACE aislib::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/aislib
)

configure_package_config_file(
    cmake/aislibConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/aislibConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/aislib
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/aislibConfigVersion.cmake
    VERSION       ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/aislibConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/aislibConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/aislib
)