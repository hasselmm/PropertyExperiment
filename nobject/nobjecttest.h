#ifndef NPROPERTY_NOBJECTTEST_H
#define NPROPERTY_NOBJECTTEST_H

#include "nproperty.h"

#include <QObject>
#include <QString>

namespace nproperty {

class HelloWorld : public Object<HelloWorld>
{
    N_OBJECT

public:
    N_PROPERTY(int, hello)          = 1;
    N_PROPERTY(int, world, Write)   = 2;
};


class NObjectBase : public QObject // for testing base type mechanism
{
    Q_OBJECT

public:
    using QObject::QObject;
};


class NObject : public Object<NObject, NObjectBase>
{
    N_OBJECT

public:
    using Object::Object;

    void modifyNotifying()
    {
        notifying = u"I have been changed per method"_qs;
    }

    N_PROPERTY(QString, constant)          = u"I am constant"_qs;
    N_PROPERTY(QString, notifying, Notify) = u"I am observing"_qs;
    N_PROPERTY(QString, writable,  Write)  = u"I am modifiable"_qs;

    // Setter<QString> setWritable = &writable;

    // static constexpr Signal<&MObject::notifying> notifyingChanged = {};
    // static constexpr Signal<&MObject::writable> writableChanged = {};
};

} // namespace nproperty

#endif // NPROPERTY_NOBJECTTEST_H
