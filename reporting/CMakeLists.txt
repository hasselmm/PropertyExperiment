add_executable(
    Reporting
    githubwriter.cpp
    githubwriter.h
    junitwriter.cpp
    junitwriter.h
    lightxmlreader.cpp
    lightxmlreader.h
    markdownwriter.cpp
    markdownwriter.h
    markup.cpp
    markup.h
    reporting.cpp
    reporting.h
)

target_link_libraries(
    Reporting PRIVATE
    Qt::Network
)

set_target_properties(
    Reporting PROPERTIES
    OUTPUT_NAME reporting
)
