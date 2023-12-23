# FIXME support other languages than C++
# FIXME add support for add_compile_definitions(), add_link_options()
# FIXME add support for target_compile_definitions(), target_compile_definitions(), target_link_options()
# NOTE  the canonical approach would be generator expressions, but they are impossible to read,
#       and even worse they tend to fail with additional tools, like clang_tidy

function(codepoetry_add_compile_options)
    set(enabled FALSE)
    set(options CLANG GNU MSVC)
    set(onevalue CLANG_MINIMUM_VERSION GNU_MINIMUM_VERSION MSVC_MINIMUM_VERSION)
    set(multivalue)

    cmake_parse_arguments(OPTIONS "${options}" "${onevalue}" "${multivalue}" ${ARGN})

    if (NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU|MSVC")
        message(WARNING "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}")
        return()
    endif()

    string(TOUPPER "${CMAKE_CXX_COMPILER_ID}" uppercase_compiler_id)
    set(minimum_version "${OPTIONS_${uppercase_compiler_id}_MINIMUM_VERSION}")

    if (minimum_version)
        if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL minimum_version)
            message(
                STATUS "Enabling compiler options for ${CMAKE_CXX_COMPILER_ID} "
                "${CMAKE_CXX_COMPILER_VERSION}: ${OPTIONS_UNPARSED_ARGUMENTS}")

            add_compile_options(${OPTIONS_UNPARSED_ARGUMENTS})
        endif()
    elseif(${OPTIONS_${uppercase_compiler_id}})
        message(
            STATUS "Enabling compiler options for ${CMAKE_CXX_COMPILER_ID}: "
            "${OPTIONS_UNPARSED_ARGUMENTS}")

        add_compile_options(${OPTIONS_UNPARSED_ARGUMENTS})
    else()
        message(
            VERBOSE "Ignoring compiler options for ${CMAKE_CXX_COMPILER_ID} "
            "${CMAKE_CXX_COMPILER_VERSION}: ${OPTIONS_UNPARSED_ARGUMENTS}")
    endif()
endfunction()
