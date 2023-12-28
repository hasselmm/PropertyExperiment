add_library(
    SObjectTest STATIC
    sobjecttest.cpp
    sobjecttest.h
)

target_link_libraries(
    SObjectTest PUBLIC
    Qt::Core
)

