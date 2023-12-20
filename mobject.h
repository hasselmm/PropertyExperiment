#ifndef MOBJECT_H
#define MOBJECT_H

#include "mproperty.h"

#include <QObject>
#include <QString>

namespace mproperty {

using namespace Qt::Literals;

class MObjectBase : public QObject // for testing base type mechanism
{
    Q_OBJECT

public:
    using QObject::QObject;
};

class MObject : public Object<MObject, MObjectBase>
{
    M_OBJECT

public:
    using Object::Object;

    void modifyNotifying()
    {
        notifying = u"I have been changed per method"_s;
    }

    Property<n(), QString>          constant = u"I am constant"_s;
    Property<n(), QString, Notify> notifying = u"I am observing"_s;
    Property<n(), QString,  Write>  writable = u"I am modifiable"_s;

    Setter<QString> setWritable = &writable;

    static constexpr Signal<&MObject::notifying> notifyingChanged = {};
    static constexpr Signal<&MObject::writable> writableChanged = {};
};

// FIXME The n() function is just a fancy way to generate generate
// a number that's unique within this class. Any number would do.
// One could manually put number literals there. The n() function
// returns the from current line number. Maybe there is some meta
// programming magic to get entirely rid of it.

} // namespace mproperty

#endif // MOBJECT_H
