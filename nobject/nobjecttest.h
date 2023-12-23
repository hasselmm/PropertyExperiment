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

    // FIXME: implement writers
    // Setter<QString> setWritable = &writable;

    // FIXME: implement signals
    // static constexpr Signal<&MObject::notifying> notifyingChanged = {};
    // static constexpr Signal<&MObject::writable> writableChanged = {};

    // FIXME: implement custom getters/setters
    // N_PROPERTY(int, customReader) = {&NObjectMacro::customInt};
    // N_PROPERTY(double, customWriter) = {reset   = &NObjectMacro::resetCustomDouble
    //                                     writer  = &NObjectMacro::setCustomDouble,
    //                                     reader  = &NObjectMacro::customDuble,
    //                                     initial =  42};

    // FIXME: but maybe generate just exactly that?
    void setWritable(QString newValue) { writable.setValue(std::move(newValue)); }

    // FIXME: remove fake signals
    void notifyingChanged(QString) {}
    void writableChanged(QString) {}
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

    Property<QString, name(__LINE__, "constant")>          constant  = u"I am constant"_qs;
    Property<QString, name(__LINE__, "notifying"), Notify> notifying = u"I am observing"_qs;
    Property<QString, name(__LINE__, "writable"),  Write>  writable  = u"I am modifiable"_qs;
};

} // namespace nproperty

#endif // NPROPERTY_NOBJECTTEST_H
