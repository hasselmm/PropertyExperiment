#ifndef AOBJECT_H
#define AOBJECT_H

#include "aproperty.h"

#include <QObject>
#include <QString>

namespace aproperty {

using namespace Qt::Literals;

class AObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString constant READ constant CONSTANT FINAL)
    Q_PROPERTY(QString notifying READ notifying NOTIFY notifyingChanged FINAL)
    Q_PROPERTY(QString writable READ writable WRITE setWritable NOTIFY writableChanged FINAL)

public:
    using QObject::QObject;

    void modifyNotifying() { notifying = u"I have been changed per method"_s; }

signals:
    void notifyingChanged(QString notifying);
    void writableChanged(QString writable);

public:
    Property<QString>                     constant  = {u"I am constant"_s};
    Notifying<&AObject::notifyingChanged> notifying = {this, u"I am observing"_s};
    Notifying<&AObject::writableChanged>  writable  = {this, u"I am modifiable"_s};
    const Setter<QString>              setWritable  = {&writable};
};

} // namespace aproperty

#endif // AOBJECT_H
