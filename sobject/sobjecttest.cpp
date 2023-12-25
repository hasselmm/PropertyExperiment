#include "sobjecttest.h"

namespace sproperty {

QString SObject::constant() const
{
    return m_constant;
}

void SObject::modifyNotifying()
{
    m_notifying = u"I have been changed per method"_qs;
    emit notifyingChanged(m_notifying);
}

QString SObject::notifying() const
{
    return m_notifying;
}

void SObject::setWritable(QString newWritable)
{
    if (std::exchange(m_writable, std::move(newWritable)) != m_writable)
        emit writableChanged(m_writable);
}

QString SObject::writable() const
{
    return m_writable;
}

} // namespace sproperty

#include "moc_sobjecttest.cpp"
