cmake_minimum_required(VERSION 3.14)

project(
    PropertyExperiment
    LANGUAGES CXX
    HOMEPAGE_URL "https://github.com/hasselmm/PropertyExperiment/")

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

find_package(Qt6 REQUIRED COMPONENTS Core Network Test)
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
    -Werror=unused-result
    CLANG GNU
)

codepoetry_add_link_options(
    -fstack-protector
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
codepoetry_add_compile_options(CLANG -Wno-gnu-zero-variadic-macro-arguments)

# Clang better uses libc++ for proper ranges support.
# For Clang 15 one also has to opt-in for experimental features.
codepoetry_add_compile_options(CLANG_EXACT_VERSION   15 -fexperimental-library)
codepoetry_add_compile_options(CLANG_MAXIMUM_VERSION 15 -stdlib=libc++)
codepoetry_add_link_options   (CLANG_MAXIMUM_VERSION 15 -stdlib=libc++)

# Compiler options for Microsoft Visual C++
# https://learn.microsoft.com/en-us/cpp/build/reference/compiler-options
codepoetry_add_compile_options(MSVC /W3 /permissive- /utf-8)

include_directories(experiment)

add_subdirectory(aobject)
add_subdirectory(mobject)
add_subdirectory(nobject)
add_subdirectory(sobject)

add_subdirectory(reporting)

add_executable(
    PropertyExperiment
    experiment/backports.cpp
    experiment/backports.h
    experiment/experiment.cpp
    experiment/experiment.h
    main.cpp
    README.md
    LICENSE
)

target_link_libraries(
    PropertyExperiment
    AObjectTest
    MObjectTest
    NObjectTest
    SObjectTest
    Qt::Test
)

set(TEST_FLAGS)

option(GENERATE_TEST_REPORTS "" NO) 

if (GENERATE_TEST_REPORTS)
    set(COMPILER_INFO "${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")

    if (DEFINED ENV{MSYSTEM})
        string(APPEND COMPILER_INFO " $ENV{MSYSTEM}")
    endif()

    file(CONFIGURE OUTPUT "compiler-info.txt" CONTENT "${COMPILER_INFO}")
    list(APPEND TEST_FLAGS -lightxml -o "test-report-${VARIANT_ID}-$<CONFIG>.xml")
endif()

add_compile_definitions(PROJECT_HOMEPAGE_URL="${CMAKE_PROJECT_HOMEPAGE_URL}")

add_test(
    NAME PropertyExperiment
    COMMAND $<TARGET_FILE:PropertyExperiment> ${TEST_FLAGS}
)

install(
    TARGETS PropertyExperiment
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

add_custom_target(
    Github SOURCES
    .github/workflows/autotest-clang.yml
    .github/workflows/autotest-gcc.yml
    .github/workflows/autotest-msvc.yml
    .github/workflows/autotest.yml
    .github/workflows/reporting.yml
)
