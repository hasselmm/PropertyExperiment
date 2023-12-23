cmake_minimum_required(VERSION 3.14)

project(PropertyExperiment LANGUAGES CXX)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(CodePoetryCompileOptions)
include(GNUInstallDirs)
enable_testing()

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Test)

# https://releases.llvm.org/17.0.1/tools/clang/docs/DiagnosticsReference.html
# https://gcc.gnu.org/onlinedocs/gcc-13.2.0/gcc/Warning-Options.html
codepoetry_add_compile_options(
    -Werror=all
    -Werror=arith-conversion
    -Werror=comment
    -Werror=conversion
    -Werror=extra
    -Werror=pedantic
    -Werror=sign-compare
    -Werror=sign-conversion
    -Werror=switch
    -Werror=unused
    CLANG GNU
)

codepoetry_add_compile_options(
    -Werror=infinite-recursion
    CLANG_MINIMUM_VERSION 15
    GNU_MINIMUM_VERSION 12
)

codepoetry_add_compile_options(
    -Werror=invalid-constexpr
    CLANG_MINIMUM_VERSION 15
    GNU_MINIMUM_VERSION 13
)

# https://learn.microsoft.com/en-us/cpp/build/reference/compiler-options
codepoetry_add_compile_options(
    /W4 /permissive- /utf-8
    MSVC)

add_executable(
    PropertyExperiment
    main.cpp

    aobject/aobject.cpp     aobject/aobject.h
    aobject/aproperty.cpp   aobject/aproperty.h
    mobject/mobject.cpp     mobject/mobject.h
    mobject/mproperty.cpp   mobject/mproperty.h
    sobject/sobject.cpp     sobject/sobject.h
)

target_link_libraries(
    PropertyExperiment
    Qt::CorePrivate
    Qt::Test
)

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
    .github/workflows/cmake-build.yml
)
