#ifndef NPROPERTY_NOBJECTTEST_H
#define NPROPERTY_NOBJECTTEST_H

#include "nmetaobject.h"
#include "nproperty.h"

#include <QObject>
#include <QString>

namespace nproperty {

/// A very basic example demonstrating the concept of NObject
///
class HelloWorld
    : public Object<HelloWorld>
{
    N_OBJECT

public:
    N_PROPERTY(int, hello)          = 1;
    N_PROPERTY(int, world, Write)   = 2;
};


/// This class simply helps testing if the generated meta object
/// provides proper information about the class hierarchy.
///
class NObjectBase
    : public QObject // for testing base type mechanism
{
    Q_OBJECT

public:
    using QObject::QObject;
};


/// This class implements an NObject based QObject using macros.
///
class NObjectMacro
    : public Object<NObjectMacro, NObjectBase>
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

    N_PROPERTY_NOTIFICATION(notifying)
    N_PROPERTY_NOTIFICATION(writable)

    N_PROPERTY_SETTER(setWritable, writable)
};

/// This class implements an NObject based QObject using C++ 20.
///
class NObjectModern
    : public Object<NObjectModern, NObjectBase>
{
    N_OBJECT

public:
    using Object::Object;

    void modifyNotifying()
    {
        notifying = u"I have been changed per method"_qs;
    }

    Property<QString, name("constant")>          constant  = u"I am constant"_qs;
    Property<QString, name("notifying"), Notify> notifying = u"I am observing"_qs;
    Property<QString, name("writable"),  Write>  writable  = u"I am modifiable"_qs;

    static constexpr Signal<&NObjectModern::notifying> notifyingChanged = {};
    static constexpr Signal<&NObjectModern::writable>  writableChanged  = {};

    void setWritable(QString newValue) { writable.setValue(std::move(newValue)); }

private:
    N_REGISTER_PROPERTY(constant);
    N_REGISTER_PROPERTY(notifying);
    N_REGISTER_PROPERTY(writable);
};

/// This class implements an NObject based QObject using C++ 20.
//
class NObjectLegacy
    : public Object<NObjectLegacy, NObjectBase>
{
    N_OBJECT

public:
    using Object::Object;

    void modifyNotifying()
    {
        notifying = u"I have been changed per method"_qs;
    }

    Property<QString, name("constant", __LINE__)>          constant  = u"I am constant"_qs;
    Property<QString, name("notifying", __LINE__), Notify> notifying = u"I am observing"_qs;
    Property<QString, name("writable", __LINE__),  Write>  writable  = u"I am modifiable"_qs;

private:
    N_REGISTER_PROPERTY(constant);
    N_REGISTER_PROPERTY(notifying);
    N_REGISTER_PROPERTY(writable);
};

} // namespace nproperty

#endif // NPROPERTY_NOBJECTTEST_H
