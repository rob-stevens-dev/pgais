cmake_minimum_required(VERSION 3.20)
project(pgais VERSION 0.2.0 LANGUAGES CXX)

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
# Provides the Phase 1 infrastructure: error handling, bit-field engine,
# payload codec, sentence framing, and the message base class with registry.
#
# Warning flags are applied only to pgais_core and the test executables via
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
# Messages library target
#
# Provides Phase 2: concrete AIS message type implementations for types
# 1–5, 11, 18, and 24, plus the registry initialisation function.
# Depends on pgais_core.
# ----------------------------------------------------------------------------
add_library(pgais_messages
    src/messages/msg1_3.cpp
    src/messages/msg4_11.cpp
    src/messages/msg05.cpp
    src/messages/msg09.cpp
    src/messages/msg10.cpp
    src/messages/msg18.cpp
    src/messages/msg19.cpp
    src/messages/msg24.cpp
    src/messages/msg27.cpp
    src/registry_init.cpp
)

target_include_directories(pgais_messages
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(pgais_messages
    PUBLIC pgais_core
)

target_compile_options(pgais_messages PRIVATE
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

set_target_properties(pgais_messages PROPERTIES
    VERSION   ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# ----------------------------------------------------------------------------
# Installation rules
# ----------------------------------------------------------------------------
include(GNUInstallDirs)

install(TARGETS pgais_core pgais_messages
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
    add_subdirectory(tests/messages)
endif()
