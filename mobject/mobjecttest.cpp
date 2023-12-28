#include "mobjecttest.h"

namespace mproperty {

using mpropertytest::MObjectTest;

template<>
std::vector<MObjectTest::MetaProperty> MObjectTest::MetaObject::makeProperties()
{
    return {
        {"constant",    &null()->constant},     // FIXME: with some additional effort
        {"notifying",   &null()->notifying},    // &MObject::constant should be possible
        {"writable",    &null()->writable},
    };
}

} // namespace mproperty

namespace mpropertytest {
M_OBJECT_IMPLEMENTATION(MObjectTest, constant, notifying, writable)
} // namespace mpropertytest

#include "moc_mobjecttest.cpp"
