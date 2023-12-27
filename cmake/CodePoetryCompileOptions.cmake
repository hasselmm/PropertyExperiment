# FIXME support other languages than C++
# FIXME add support for target_compile_definitions(), target_compile_definitions(), target_link_options()
# NOTE  the canonical approach would be generator expressions, but they are impossible to read,
#       and even worse they tend to fail with additional tools, like clang_tidy

include(CodePoetryNormalizeVersion)

function(codepoetry_check_compiler_flags RESULT_VARIABLE OPTIONS_VARIABLE)
    set(enabled FALSE)
    set(options CLANG GNU MSVC)
    set(onevalue
        CLANG_EXACT_VERSION   GNU_EXACT_VERSION   MSVC_EXACT_VERSION
        CLANG_MINIMUM_VERSION GNU_MINIMUM_VERSION MSVC_MINIMUM_VERSION
        CLANG_MAXIMUM_VERSION GNU_MAXIMUM_VERSION MSVC_MAXIMUM_VERSION)
    set(multivalue)

    cmake_parse_arguments(OPTIONS "${options}" "${onevalue}" "${multivalue}" ${ARGN})

    if (NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU|MSVC")
        message(WARNING "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}")
        return()
    endif()

    string(TOUPPER "${CMAKE_CXX_COMPILER_ID}" uppercase_compiler_id)

    set(exact_version   "${OPTIONS_${uppercase_compiler_id}_EXACT_VERSION}")
    set(minimum_version "${OPTIONS_${uppercase_compiler_id}_MINIMUM_VERSION}")
    set(maximum_version "${OPTIONS_${uppercase_compiler_id}_MAXIMUM_VERSION}")

    set(acceptance)
    set(rejected NO)

    if (exact_version)
        codepoetry_normalize_version("${CMAKE_CXX_COMPILER_VERSION}" "${exact_version}" normalized_version)

        if (normalized_version VERSION_EQUAL exact_version)
            list(APPEND acceptance "(=${exact_version})")
        else()
            set(rejected YES)
        endif()
    endif()

    if (minimum_version)
        codepoetry_normalize_version("${CMAKE_CXX_COMPILER_VERSION}" "${minimum_version}" normalized_version)

        if (normalized_version VERSION_GREATER_EQUAL minimum_version)
            list(APPEND acceptance "(>=${minimum_version})")
        else()
            set(rejected YES)
        endif()
    endif()

    if (maximum_version)
        codepoetry_normalize_version("${CMAKE_CXX_COMPILER_VERSION}" "${maximum_version}" normalized_version)

        if (normalized_version VERSION_LESS_EQUAL maximum_version)
            list(APPEND acceptance "(<=${maximum_version})")

        else()
            set(rejected YES)
        endif()
    endif()

    if("${OPTIONS_${uppercase_compiler_id}}" OR "${acceptance}")
        list(PREPEND acceptance "${CMAKE_CXX_COMPILER_ID}")
    endif()

    set("${RESULT_VARIABLE}" "${acceptance}" PARENT_SCOPE)
    set("${OPTIONS_VARIABLE}" "${OPTIONS_UNPARSED_ARGUMENTS}" PARENT_SCOPE)
endfunction()

function(codepoetry_add_compile_options)
    codepoetry_check_compiler_flags(accepted options ${ARGV})

    if (accepted)
        message(VERBOSE "Adding compile options for ${accepted}: ${options}")
        add_compile_options(${options})
    else()
        message(VERBOSE "Ignoring compile options for ${CMAKE_CXX_COMPILER_ID}: ${options}")
    endif()
endfunction()

function(codepoetry_add_compile_definitions)
    codepoetry_check_compiler_flags(accepted options ${ARGV})

    if (accepted)
        message(VERBOSE "Adding compile definitions for ${accepted}: ${options}")
        add_compile_definitions(${options})
    else()
        message(VERBOSE "Ignoring compile options for ${CMAKE_CXX_COMPILER_ID}: ${options}")
    endif()
endfunction()

function(codepoetry_add_link_options)
    codepoetry_check_compiler_flags(accepted options ${ARGV})

    if (accepted)
        message(VERBOSE "Adding link options for ${accepted}: ${options}")
        add_link_options(${options})
    else()
        message(VERBOSE "Ignoring link options for ${CMAKE_CXX_COMPILER_ID}: ${options}")
    endif()
endfunction()
