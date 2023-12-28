add_library(
    SObjectTest STATIC
    sobjecttest.cpp
    sobjecttest.h
    README.md
)

target_link_libraries(
    SObjectTest PUBLIC
    Qt::Core
)

