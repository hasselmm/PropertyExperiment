macro(codepoetry_assert ACTUAL_VALUE OPERATOR EXPECTED_VALUE)
    if (NOT "${ACTUAL_VALUE}" ${OPERATOR} "${EXPECTED_VALUE}")
        message(FATAL_ERROR "Assertion failed: '${ACTUAL_VALUE}' ${OPERATOR} '${EXPECTED_VALUE}'")
    else()
        message(DEBUG "Assertion passed: '${ACTUAL_VALUE}' ${OPERATOR} '${EXPECTED_VALUE}'")
    endif()
endmacro()
