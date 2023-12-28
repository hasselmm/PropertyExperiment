add_library(
    AObjectTest STATIC
    aobjecttest.cpp
    aobjecttest.h
    aproperty.cpp
    aproperty.h
)

target_link_libraries(
    AObjectTest PUBLIC
    Qt::Core
)
