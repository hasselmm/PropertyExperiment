add_executable(
    Reporting
    junitwriter.cpp
    junitwriter.h
    lightxmlreader.cpp
    lightxmlreader.h
    markdownwriter.cpp
    markdownwriter.h
    reporting.cpp
    reporting.h
    githubwriter.h githubwriter.cpp
)

target_link_libraries(
    Reporting PRIVATE
    Qt::Network
)

set_target_properties(
    Reporting PROPERTIES
    OUTPUT_NAME reporting
)
