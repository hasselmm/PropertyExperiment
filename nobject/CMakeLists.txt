add_library(
    NObjectTest STATIC
    nconcepts.cpp
    nconcepts.h
    nlinenumber.cpp
    nlinenumber_p.h
    nmetaenum.cpp
    nmetaenum.h
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

