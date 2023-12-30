#ifndef APROPERTY_AOBJECTTEST_H
#define APROPERTY_AOBJECTTEST_H

#include "aproperty.h"
#include "experiment.h"

#include <QObject>
#include <QString>

namespace apropertytest {

using aproperty::Property;
using aproperty::Notifying;
using aproperty::Setter;

class AObjectTest
    : public experiment::ParentClass
    , public experiment::InterfaceOne
    , public experiment::InterfaceTwo
{
    Q_OBJECT
    Q_PROPERTY(QString constant READ constant CONSTANT FINAL)
    Q_PROPERTY(QString notifying READ notifying NOTIFY notifyingChanged FINAL)
    Q_PROPERTY(QString writable READ writable WRITE setWritable NOTIFY writableChanged FINAL)

    Q_CLASSINFO("URL", "https://github.com/hasselmm/PropertyExperiment/")
    Q_INTERFACES(experiment::InterfaceOne experiment::InterfaceTwo)

public:
    using ParentClass::ParentClass;

    void modifyNotifying() { notifying = u"I have been changed per method"_qs; }

signals:
    void notifyingChanged(QString notifying);
    void writableChanged(QString writable);

public:
    const char *firstInterfaceCall() const override { return "first"; }
    const char *secondInterfaceCall() const override { return "second"; }

public:
    Property<QString>                         constant     = {u"I am constant"_qs};
    Notifying<&AObjectTest::notifyingChanged> notifying    = {this, u"I am observing"_qs};
    Notifying<&AObjectTest::writableChanged>  writable     = {this, u"I am modifiable"_qs};
    const Setter<QString>                     setWritable  = {&writable};
};

} // namespace aproperty

#endif // APROPERTY_AOBJECTTEST_H
