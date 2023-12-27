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

find_program(CCACHE_EXECUTABLE ccache)

if (CCACHE_EXECUTABLE)
    message(STATUS "Using ccache: ${CCACHE_EXECUTABLE}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_EXECUTABLE}")
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Test)
message(STATUS "Using Qt ${Qt6_VERSION}")

# https://releases.llvm.org/17.0.1/tools/clang/docs/DiagnosticsReference.html
# https://gcc.gnu.org/onlinedocs/gcc-13.2.0/gcc/Warning-Options.html
codepoetry_add_compile_options(
    -fstack-protector
    -Werror=all
    -Werror=comment
    -Werror=conversion
    -Werror=extra
    -Werror=format
    -Werror=pedantic
    -Werror=reorder
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

codepoetry_add_compile_options(
    -Werror=arith-conversion
    GNU
)

# Clang is wrong about this: C++ 20 allows empty variadic argument lists
# for macros: https://eel.is/c++draft/cpp.replace.general#5
# https://github.com/llvm/llvm-project/issues/76375
codepoetry_add_compile_options(
    -Wno-gnu-zero-variadic-macro-arguments
    CLANG
)

# https://learn.microsoft.com/en-us/cpp/build/reference/compiler-options
codepoetry_add_compile_options(
    /W3 /permissive- /utf-8
    MSVC)

add_executable(
    PropertyExperiment
    main.cpp

    aobject/aobjecttest.cpp
    aobject/aobjecttest.h
    aobject/aproperty.cpp
    aobject/aproperty.h
    mobject/mobjecttest.cpp
    mobject/mobjecttest.h
    mobject/mproperty.cpp
    mobject/mproperty.h
    sobject/sobjecttest.cpp
    sobject/sobjecttest.h
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
    .github/workflows/autotest-clang.yml
    .github/workflows/autotest-gcc.yml
    .github/workflows/autotest-msvc.yml
    .github/workflows/autotest.yml
)
