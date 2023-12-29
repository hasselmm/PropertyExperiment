add_library(
    NObjectTest STATIC
    nlinenumber.cpp
    nlinenumber_p.h
    nmetaobject.cpp
    nmetaobject.h
    nmetaobject_p.h
    nobjecttest.cpp
    nobjecttest.h
    nproperty.cpp
    nproperty.h
    nproperty_p.h
    ntypetraits.cpp
    ntypetraits.h
    README.md
)

target_link_libraries(
    NObjectTest PUBLIC
    Qt::CorePrivate
)

