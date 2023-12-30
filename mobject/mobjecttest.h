#ifndef MOBJECTTEST_H
#define MOBJECTTEST_H

#include "experiment.h"
#include "mproperty.h"

#include <QObject>
#include <QString>

namespace mpropertytest {

using enum mproperty::Feature;
using      mproperty::Signal;

class MObjectTest : public mproperty::Object<MObjectTest, experiment::ParentClass>
{
    M_OBJECT

public:
    using Object::Object;

    void modifyNotifying()
    {
        notifying = u"I have been changed per method"_qs;
    }

    Property<n(), QString>          constant = u"I am constant"_qs;
    Property<n(), QString, Notify> notifying = u"I am observing"_qs;
    Property<n(), QString,  Write>  writable = u"I am modifiable"_qs;

    Setter<QString> setWritable = &writable;

    static constexpr Signal<&MObjectTest::notifying> notifyingChanged = {};
    static constexpr Signal<&MObjectTest::writable>  writableChanged  = {};
};

// FIXME The n() function is just a fancy way to generate generate
// a number that's unique within this class. Any number would do.
// One could manually put number literals there. The n() function
// returns the from current line number. Maybe there is some meta
// programming magic to get entirely rid of it.

} // namespace mpropertytest

#endif // MOBJECTTEST_H
