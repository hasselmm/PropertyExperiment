add_library(
    MObjectTest STATIC
    mobjecttest.cpp
    mobjecttest.h
    mproperty.cpp
    mproperty.h
    README.md
)

target_link_libraries(
    MObjectTest PUBLIC
    Qt::CorePrivate
)

