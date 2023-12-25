#ifndef SPROPERTY_SOBJECT_H
#define SPROPERTY_SOBJECT_H

#include <QObject>

namespace sproperty {

class SObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString constant READ constant CONSTANT FINAL)
    Q_PROPERTY(QString notifying READ notifying NOTIFY notifyingChanged FINAL)
    Q_PROPERTY(QString writable READ writable WRITE setWritable NOTIFY writableChanged FINAL)

public:
    using QObject::QObject;

    QString constant() const;

    void modifyNotifying();
    QString notifying() const;

    void setWritable(QString newWritable);
    QString writable() const;

signals:
    void notifyingChanged(QString notifying);
    void writableChanged(QString writable);

private:
    QString m_constant  = u"I am constant"_qs;
    QString m_notifying = u"I am observing"_qs;
    QString m_writable  = u"I am modifiable"_qs;
};

} // namespace sproperty

#endif // SPROPERTY_SOBJECT_H
