#include "sobjecttest.h"

namespace spropertytest {

QString SObjectTest::constant() const
{
    return m_constant;
}

void SObjectTest::modifyNotifying()
{
    m_notifying = u"I have been changed per method"_qs;
    emit notifyingChanged(m_notifying);
}

QString SObjectTest::notifying() const
{
    return m_notifying;
}

void SObjectTest::setWritable(QString newWritable)
{
    if (std::exchange(m_writable, std::move(newWritable)) != m_writable)
        emit writableChanged(m_writable);
}

QString SObjectTest::writable() const
{
    return m_writable;
}

} // namespace spropertytest

#include "moc_sobjecttest.cpp"
