cmake_minimum_required(VERSION 3.14)

project(strap LANGUAGES C)

option(STRAP_BUILD_TESTS "Build STRAP tests" ON)
option(STRAP_BUILD_BENCHMARKS "Build STRAP benchmarks" OFF)

add_library(strap STATIC strap.c)
target_include_directories(
    strap
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>)
target_compile_features(strap PRIVATE c_std_99)

if(MSVC)
    target_compile_options(strap PRIVATE /W4 /permissive-)
    target_compile_definitions(
        strap
        PRIVATE
            _CRT_SECURE_NO_WARNINGS
            _CRT_NONSTDC_NO_DEPRECATE)
else()
    target_compile_options(strap PRIVATE -Wall -Wextra)
endif()

if(STRAP_BUILD_TESTS)
    enable_testing()
    add_executable(strap_tests tests/test_strap.c)
    target_link_libraries(strap_tests PRIVATE strap)
    if(MSVC)
        target_compile_definitions(
            strap_tests
            PRIVATE
                _CRT_SECURE_NO_WARNINGS
                _CRT_NONSTDC_NO_DEPRECATE)
    endif()
    add_test(NAME strap_tests COMMAND strap_tests)
endif()

if(STRAP_BUILD_BENCHMARKS)
    add_executable(strap_bench benchmarks/strap_bench.c)
    target_link_libraries(strap_bench PRIVATE strap)
    if(MSVC)
        target_compile_definitions(
            strap_bench
            PRIVATE
                _CRT_SECURE_NO_WARNINGS
                _CRT_NONSTDC_NO_DEPRECATE)
    endif()
endif()

include(GNUInstallDirs)
install(TARGETS strap
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES strap.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
