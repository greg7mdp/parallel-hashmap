cmake_minimum_required(VERSION 3.8)

list (APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(DetectVersion)

project(phmap VERSION ${DETECTED_PHMAP_VERSION} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(PHMAP_HEADERS phmap.h phmap_base.h phmap_bits.h phmap_utils.h phmap_config.h)
set(PHMAP_DIR parallel_hashmap)
set(CMAKE_SUPPRESS_REGENERATION true)            ## suppress ZERO_CHECK project

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(helpers)

add_library(${PROJECT_NAME} INTERFACE)

target_sources(${PROJECT_NAME} INTERFACE ${PHMAP_HEADERS})

target_include_directories(
     ${PROJECT_NAME} INTERFACE
     $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/${PHMAP_DIR}>
     $<INSTALL_INTERFACE:include>)

install(
    DIRECTORY ${PROJECT_SOURCE_DIR}/${PHMAP_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PHMAP_DIR})

install(TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}-targets)

export(EXPORT ${PROJECT_NAME}-targets
       FILE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Targets.cmake")

## ------------------------- building tests and examples -------------
option(PHMAP_BUILD_TESTS    "Whether or not to build the tests"    OFF)
option(PHMAP_BUILD_EXAMPLES "Whether or not to build the examples" OFF)

if (PHMAP_BUILD_TESTS)
    #add_subdirectory(tests)
    include(cmake/DownloadGTest.cmake)

    check_target(gtest)
    check_target(gtest_main)
    check_target(gmock)

    list(APPEND ABSL_TEST_COMMON_LIBRARIES
         gtest_main
         gtest
         gmock
         ${CMAKE_THREAD_LIBS_INIT}
    )

absl_cc_test(
  NAME
    flat_hash_set_test
  SRCS
    "tests/flat_hash_set_test.cc"
  COPTS
    "-DUNORDERED_SET_CXX17"
  DEPS
    gmock_main
)

endif()

if (PHMAP_BUILD_EXAMPLES)
    add_executable(basic examples/basic.cc)
endif()


