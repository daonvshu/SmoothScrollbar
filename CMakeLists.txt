cmake_minimum_required(VERSION 3.21)

project(SmoothScrollbar
    VERSION 1.0.0
    DESCRIPTION "Input-aware smooth scrolling for Qt Widgets"
    LANGUAGES CXX
)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(GenerateExportHeader)

option(SMOOTHSCROLLBAR_BUILD_EXAMPLES "Build the widgets example" ON)
option(SMOOTHSCROLLBAR_BUILD_TESTS "Build the test suite" ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

add_library(SmoothScrollbar
    include/smoothscroll/smoothscrollcontroller.h
    include/smoothscroll/smoothscrollsettings.h
    src/qscrolleradapter_p.cpp
    src/qscrolleradapter_p.h
    src/scrollaxisanimator_p.cpp
    src/scrollaxisanimator_p.h
    src/smoothscrollcontroller.cpp
    src/wheeldeltanormalizer_p.cpp
    src/wheeldeltanormalizer_p.h
)
add_library(SmoothScrollbar::SmoothScrollbar ALIAS SmoothScrollbar)

set_target_properties(SmoothScrollbar PROPERTIES
    AUTOMOC ON
    CXX_EXTENSIONS OFF
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)
target_compile_features(SmoothScrollbar PUBLIC cxx_std_17)
target_link_libraries(SmoothScrollbar PUBLIC Qt${QT_VERSION_MAJOR}::Widgets)
target_include_directories(SmoothScrollbar
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

generate_export_header(SmoothScrollbar
    BASE_NAME SMOOTHSCROLLBAR
    EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/include/smoothscroll/smoothscrollbar_export.h
)
if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(SmoothScrollbar PUBLIC SMOOTHSCROLLBAR_STATIC_DEFINE)
endif()

if(MSVC)
    target_compile_options(SmoothScrollbar PRIVATE /W4 /permissive-)
else()
    target_compile_options(SmoothScrollbar PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(SMOOTHSCROLLBAR_BUILD_EXAMPLES)
    add_subdirectory(examples/widgets-demo)
endif()

if(SMOOTHSCROLLBAR_BUILD_TESTS)
    include(CTest)
    if(BUILD_TESTING)
        find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test)
        add_subdirectory(tests)
    endif()
endif()

install(TARGETS SmoothScrollbar
    EXPORT SmoothScrollbarTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/include/smoothscroll/smoothscrollbar_export.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/smoothscroll
)
install(EXPORT SmoothScrollbarTargets
    FILE SmoothScrollbarTargets.cmake
    NAMESPACE SmoothScrollbar::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SmoothScrollbar
)

configure_package_config_file(
    cmake/SmoothScrollbarConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/SmoothScrollbarConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SmoothScrollbar
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/SmoothScrollbarConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/SmoothScrollbarConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/SmoothScrollbarConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SmoothScrollbar
)
install(FILES LICENSE README.md
    DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/SmoothScrollbar
)
