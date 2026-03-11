include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)

# Prevent GoogleTest from overriding the parent project's compiler/linker
# settings on Windows.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Silence warnings in GoogleTest's own source files.  We cannot control their
# code, and the diagnostics they emit are not actionable.
set(CMAKE_CXX_FLAGS_SAVE "${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
FetchContent_MakeAvailable(googletest)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_SAVE}")

# ----------------------------------------------------------------------------
# Core test executable
# ----------------------------------------------------------------------------
add_executable(pgais_core_tests
    test_error.cpp
    test_bitfield.cpp
    test_payload.cpp
    test_sentence.cpp
    test_assembler.cpp
    test_message.cpp
)

# pgais_messages is linked here because test_message.cpp calls
# register_all_decoders() in the RegistryIsolation::TearDown() method to
# restore the singleton to a fully-populated state after each isolation test.
# pgais_messages links pgais_core publicly, so the explicit pgais_core entry
# below is redundant but retained for clarity.
target_link_libraries(pgais_core_tests
    PRIVATE
        pgais_core
        pgais_messages
        GTest::gtest_main
)

# Apply the same strict warning set as the library, plus -Werror, to the test
# sources.  GoogleTest headers are excluded because they are marked SYSTEM.
target_compile_options(pgais_core_tests PRIVATE
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

# Mark GoogleTest include directories as SYSTEM so the compiler does not emit
# warnings from headers in those paths.
get_target_property(_gtest_inc GTest::gtest INTERFACE_INCLUDE_DIRECTORIES)
if(_gtest_inc)
    target_include_directories(pgais_core_tests SYSTEM PRIVATE ${_gtest_inc})
endif()

include(GoogleTest)
gtest_discover_tests(pgais_core_tests)