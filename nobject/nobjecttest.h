#ifndef NPROPERTY_NOBJECTTEST_H
#define NPROPERTY_NOBJECTTEST_H

#include "experiment.h"
#include "nmetaobject.h"
#include "nproperty.h"

#include <QObject>
#include <QString>

// This namespace is purposefully distinct from "nproperty".
// This helps to ensure that the definitions are self-contained,
// and that the macros fully quality their types.
namespace npropertytest {

using enum nproperty::Feature;

/// A very basic example demonstrating the concept of NObject
///
class HelloWorld : public nproperty::Object<HelloWorld>
{
    N_OBJECT

public:
    N_PROPERTY(int, hello)          = 1;
    N_PROPERTY(int, world, Write)   = 2;
};


/// This class implements an NObject based QObject using macros.
///
class NObjectMacro : public nproperty::Object<NObjectMacro, experiment::ParentClass>
{
    N_OBJECT
    N_CLASSINFO("URL", "https://github.com/hasselmm/PropertyExperiment/")

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
class NObjectModern : public nproperty::Object<NObjectModern, experiment::ParentClass>
{
    N_OBJECT

public:
    using Object::Object;

    void modifyNotifying()
    {
        notifying = u"I have been changed per method"_qs;
    }

    Property<QString, l()>          constant = u"I am constant"_qs;
    Property<QString, l(), Notify> notifying = u"I am observing"_qs;
    Property<QString, l(),  Write>  writable = u"I am modifiable"_qs;

    static constexpr nproperty::Signal<&NObjectModern::notifying> notifyingChanged = {};
    static constexpr nproperty::Signal<&NObjectModern::writable>  writableChanged  = {};

    void setWritable(QString newValue) { writable.setValue(std::move(newValue)); }

private:
    N_REGISTER_PROPERTY(constant);
    N_REGISTER_PROPERTY(notifying);
    N_REGISTER_PROPERTY(writable);

    static consteval auto member(::nproperty::detail::Tag<l()>)
    { return makeClassInfo("URL", "https://github.com/hasselmm/PropertyExperiment/"); }
};

/// This class implements an NObject based QObject using more traditional C++
//
class NObjectLegacy : public nproperty::Object<NObjectLegacy, experiment::ParentClass>
{
    N_OBJECT

public:
    using Object::Object;

    void modifyNotifying()
    {
        notifying = u"I have been changed per method"_qs;
    }

    Property<QString, __LINE__>          constant = u"I am constant"_qs;
    Property<QString, __LINE__, Notify> notifying = u"I am observing"_qs;
    Property<QString, __LINE__,  Write>  writable = u"I am modifiable"_qs;

private:
    N_REGISTER_PROPERTY(constant);
    N_REGISTER_PROPERTY(notifying);
    N_REGISTER_PROPERTY(writable);

    static consteval auto member(::nproperty::detail::Tag<__LINE__>)
    { return makeClassInfo("URL", "https://github.com/hasselmm/PropertyExperiment/"); }
};

} // namespace npropertytest

#endif // NPROPERTY_NOBJECTTEST_H
