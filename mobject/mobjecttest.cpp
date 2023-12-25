#include "mobjecttest.h"

namespace mproperty {

template<>
std::vector<MObject::MetaProperty> MObject::MetaObject::makeProperties()
{
    return {
        {"constant",    &null()->constant},     // FIXME: with some additional effort
        {"notifying",   &null()->notifying},    // &MObject::constant should be possible
        {"writable",    &null()->writable},
    };
}

M_OBJECT_IMPLEMENTATION(MObject, constant, notifying, writable)

} // namespace mproperty

#include "moc_mobjecttest.cpp"
