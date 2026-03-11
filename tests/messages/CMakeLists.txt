include(FetchContent)

# GoogleTest is fetched by tests/core/CMakeLists.txt when both test
# directories are enabled together.  FetchContent_Declare may not be called
# twice with the same content name in a single CMake invocation, so we check
# whether the GTest targets already exist before attempting to fetch again.
# This guard also covers the case where tests/messages is configured in
# isolation without tests/core.
if(NOT TARGET GTest::gtest)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.14.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(CMAKE_CXX_FLAGS_SAVE "${CMAKE_CXX_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
    FetchContent_MakeAvailable(googletest)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_SAVE}")
endif()

# ----------------------------------------------------------------------------
# Messages test executable
# ----------------------------------------------------------------------------
add_executable(pgais_messages_tests
    test_msg1_3.cpp
    test_msg4_11.cpp
    test_msg5.cpp
    test_msg18.cpp
    test_msg24.cpp
)

target_link_libraries(pgais_messages_tests
    PRIVATE
        pgais_core
        pgais_messages
        GTest::gtest_main
)

target_compile_options(pgais_messages_tests PRIVATE
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

get_target_property(_gtest_inc GTest::gtest INTERFACE_INCLUDE_DIRECTORIES)
if(_gtest_inc)
    target_include_directories(pgais_messages_tests SYSTEM PRIVATE ${_gtest_inc})
endif()

include(GoogleTest)
gtest_discover_tests(pgais_messages_tests)