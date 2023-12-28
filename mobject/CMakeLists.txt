add_library(
    MObjectTest STATIC
    mobjecttest.cpp
    mobjecttest.h
    mproperty.cpp
    mproperty.h
)

target_link_libraries(
    MObjectTest PUBLIC
    Qt::CorePrivate
)

