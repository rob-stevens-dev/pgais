cmake_minimum_required(VERSION 3.16)
project(pgais VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ----------------------------------------------------------------------------
# Build options
# ----------------------------------------------------------------------------
option(PGAIS_BUILD_TESTS      "Build unit tests"                      ON)
option(PGAIS_BUILD_DOCS       "Build Doxygen documentation"           OFF)
option(PGAIS_ENABLE_COVERAGE  "Enable gcov coverage instrumentation"  OFF)

# ----------------------------------------------------------------------------
# Coverage instrumentation (GCC / Clang only)
# ----------------------------------------------------------------------------
if(PGAIS_ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(--coverage -O0 -fno-inline)
        add_link_options(--coverage)
    else()
        message(WARNING
            "PGAIS_ENABLE_COVERAGE requested but compiler is not GCC or Clang; ignoring.")
    endif()
endif()

# ----------------------------------------------------------------------------
# Core library target
#
# Warning flags are applied only to pgais_core and the test executable via
# target_compile_options, not globally via add_compile_options.  This prevents
# third-party headers (GoogleTest, system headers) from emitting diagnostics
# that are outside our control.
# ----------------------------------------------------------------------------
add_library(pgais_core
    src/core/error.cpp
    src/core/bitfield.cpp
    src/core/payload.cpp
    src/core/sentence.cpp
    src/core/message.cpp
)

target_include_directories(pgais_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_options(pgais_core PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wconversion
    -Wsign-conversion
    -Wnull-dereference
    -Wdouble-promotion
    -Werror
)

set_target_properties(pgais_core PROPERTIES
    VERSION   ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# ----------------------------------------------------------------------------
# Installation rules
# ----------------------------------------------------------------------------
include(GNUInstallDirs)

install(TARGETS pgais_core
    EXPORT  pgaisTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY include/aislib DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT pgaisTargets
    FILE      pgaisConfig.cmake
    NAMESPACE pgais::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/pgais
)

# ----------------------------------------------------------------------------
# Tests
# ----------------------------------------------------------------------------
if(PGAIS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests/core)
endif()