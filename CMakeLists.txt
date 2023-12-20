cmake_minimum_required(VERSION 3.14)

project(PropertyExperiment LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Test)

include(GNUInstallDirs)
enable_testing()

add_executable(
    PropertyExperiment
    main.cpp

    aobject.cpp     aobject.h
    aproperty.cpp   aproperty.h
    mobject.h       mobject.cpp
    mproperty.cpp   mproperty.h
    sobject.h sobject.cpp
)

target_link_libraries(PropertyExperiment Qt::CorePrivate Qt::Test)

add_test(
    NAME PropertyExperiment
    COMMAND $<TARGET_FILE:PropertyExperiment>
)

install(
    TARGETS PropertyExperiment
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

add_custom_target(
    Documentation SOURCES
    LICENSE README.md
)

add_custom_target(
    GitHub SOURCES
    .github\workflows\cmake-build.yml
)
