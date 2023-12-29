add_library(
    AObjectTest STATIC
    aobjecttest.cpp
    aobjecttest.h
    aproperty.cpp
    aproperty.h
    README.md
)

target_link_libraries(
    AObjectTest PUBLIC
    Qt::Core
)
