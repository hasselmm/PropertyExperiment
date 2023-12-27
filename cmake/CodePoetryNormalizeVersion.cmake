function(codepoetry_normalize_version TARGET_VERSION REFERENCE_VERSION OUTPUT_VARIABLE)
    # turn both versions into lists
    string(REPLACE "." ";" reference_version_list "${REFERENCE_VERSION}")
    string(REPLACE "." ";"    target_version_list    "${TARGET_VERSION}")

    # trim target version list to reference list's length
    list(LENGTH reference_version_list reference_version_length)
    list(SUBLIST target_version_list 0 ${reference_version_length} target_version_list)

    # report result
    string(JOIN "." TARGET_VERSION ${target_version_list})
    set("${OUTPUT_VARIABLE}" "${TARGET_VERSION}" PARENT_SCOPE)
endfunction()

include(CodePoetryAssertions)

if (CODEPOETRY_TESTING)
    codepoetry_normalize_version("15.0.6" "15"       version1)
    codepoetry_normalize_version("15.0.6" "15.0"     version2)
    codepoetry_normalize_version("15.0.6" "15.0.0"   version3)
    codepoetry_normalize_version("15.0.6" "15.0.0.0" version4)

    codepoetry_assert("${version1}" STREQUAL "15")
    codepoetry_assert("${version2}" STREQUAL "15.0")
    codepoetry_assert("${version3}" STREQUAL "15.0.6")
    codepoetry_assert("${version4}" STREQUAL "15.0.6")
endif()
