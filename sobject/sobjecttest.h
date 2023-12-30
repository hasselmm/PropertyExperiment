#ifndef SPROPERTY_SOBJECTTEST_H
#define SPROPERTY_SOBJECTTEST_H

#include "experiment.h"

#include <QObject>

namespace spropertytest {

class SObjectTest
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

    QString constant() const;

    void modifyNotifying();
    QString notifying() const;

    void setWritable(QString newWritable);
    QString writable() const;

signals:
    void notifyingChanged(QString notifying);
    void writableChanged(QString writable);

public:
    const char *firstInterfaceCall() const override { return "first"; }
    const char *secondInterfaceCall() const override { return "second"; }

private:
    QString m_constant  = u"I am constant"_qs;
    QString m_notifying = u"I am observing"_qs;
    QString m_writable  = u"I am modifiable"_qs;
};

} // namespace spropertytest

#endif // SPROPERTY_SOBJECTTEST_H
