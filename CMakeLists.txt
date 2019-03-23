cmake_minimum_required(VERSION 3.8)

project(phmap VERSION 0.9 LANGUAGES CXX)

include(GNUInstallDirs)

add_library(parallel_hashmap INTERFACE)

target_sources(parallel_hashmap INTERFACE 
     phmap.h phmap_base.h phmap_bits.h phmap_utils.h phmap_config.h)

target_include_directories(
    parallel_hashmap INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/parallel_hashmap>
    $<INSTALL_INTERFACE:include>
)

add_executable(basic examples/basic.cc)
