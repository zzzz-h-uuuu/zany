cmake_minimum_required(VERSION 3.15)

project(zany
    VERSION 1.0.0
    LANGUAGES CXX
)

add_library(zany INTERFACE)
add_library(zany::zany ALIAS zany)

target_include_directories(zany INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(zany INTERFACE cxx_std_17)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(
    TARGETS zany
    EXPORT zanyTargets
)

install(
    FILES include/zany.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    EXPORT zanyTargets
    FILE zanyTargets.cmake
    NAMESPACE zany::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/zany
)

configure_package_config_file(
    cmake/zanyConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/zanyConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/zany
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/zanyConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/zanyConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/zanyConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/zany
)

option(ZANY_BUILD_TESTS "Build zany tests" ON)

if(ZANY_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()