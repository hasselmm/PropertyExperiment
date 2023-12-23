#include "nmetaobject.h"

#include <private/qmetaobjectbuilder_p.h>

#include <QObject>
#include <QLoggingCategory>

namespace nproperty::detail {

namespace {
Q_LOGGING_CATEGORY(lcMetaObject, "nproperty.metaobject");
}

void MetaObjectData::emplace(MemberInfo &&member)
{
    if (Q_UNLIKELY(!member))
        return;

    if (member.type == MemberInfo::Type::Property)
        m_propertyOffsets.emplace_back(m_members.size());

    m_members.emplace_back(std::move(member));
}

void MetaObjectData::metaCall(QObject           *object,
                              QMetaObject::Call    call,
                              int                offset,
                              void               **args) const
{
    switch (call) {
    case QMetaObject::ReadProperty:
        readProperty(object, static_cast<std::size_t>(offset), args[0]);
        return;

    case QMetaObject::WriteProperty:
        writeProperty(object, static_cast<std::size_t>(offset), args[0]);
        return;

    case QMetaObject::ResetProperty:
        resetProperty(object, static_cast<std::size_t>(offset));
        return;

    case QMetaObject::IndexOfMethod:
        indexOfMethod(reinterpret_cast<int *>(args[0]),
                      *reinterpret_cast<void **>(args[1]));
        return;

    default:
        // The `QMetaObject::Call` enumeration grows regularly.
        // It doesn't make sense to list unhandled cases here.
        break;
    }

    qCWarning(lcMetaObject,
              "Unsupported metacall for %s: call=%d, offset=%d, args=%p",
              object->metaObject()->className(), call, offset,
              static_cast<const void *>(args));
}

const MemberInfo *MetaObjectData::propertyInfo(std::size_t offset) const noexcept
{
    if (Q_UNLIKELY(offset >= m_propertyOffsets.size()))
        return nullptr;

    const auto property = memberInfo(m_propertyOffsets.at(offset));

    Q_ASSERT(property != nullptr);
    Q_ASSERT(property->type == MemberInfo::Type::Property);

    return property;
}

const MemberInfo *MetaObjectData::memberInfo(std::size_t offset) const noexcept
{
    if (Q_UNLIKELY(offset >= m_members.size()))
        return nullptr;

    return &m_members[offset];
}

quintptr MetaObjectData::memberOffset(quintptr name) const noexcept
{
    const auto memberIndex = [](const MemberInfo &member) {
        return member.name.index;
    };

    const auto it = std::ranges::lower_bound(m_members, name, {}, memberIndex);

    if (Q_UNLIKELY(it == m_members.cend()))
        return 0;

    Q_ASSERT(it->resolveOffset);
    return it->resolveOffset();
}

void MetaObjectData::readProperty(const QObject *object, std::size_t offset, void *result) const
{
    if (const auto member = propertyInfo(offset);
        Q_LIKELY(member && member->readProperty)) {
        member->readProperty(object, result);
    } else {
        qCWarning(lcMetaObject, "No readable property at offset %zd for %s",
                  offset, object->metaObject()->className());
    }
}

void MetaObjectData::writeProperty(QObject *object, std::size_t offset, void *value) const
{
    if (const auto member = propertyInfo(offset);
        Q_LIKELY(member && member->writeProperty)) {
        member->writeProperty(object, value);
    } else {
        qCWarning(lcMetaObject, "No writable property at offset %zd for %s",
                  offset, object->metaObject()->className());
    }
}

void MetaObjectData::resetProperty(QObject *object, std::size_t offset) const
{
    if (const auto member = propertyInfo(offset);
        Q_UNLIKELY(member && member->resetProperty)) {
        member->resetProperty(object);
    } else {
        qCWarning(lcMetaObject, "No resetable property at offset %zd for %s",
                  offset, object->metaObject()->className());
    }
}

void MetaObjectData::indexOfMethod(int *, void *) const noexcept
{
    // FIXME: implement
}

const QMetaObject *MetaObjectBuilder::build(const QMetaType          &metaType,
                                            const QMetaObject      *superClass,
                                            const MetaObjectData   *objectData,
                                            MetaCallFunction  metaCallFunction)
{
    auto metaObject = QMetaObjectBuilder{};

    metaObject.setFlags(PropertyAccessInStaticMetaCall);
    metaObject.setClassName(metaType.name());
    metaObject.setSuperClass(superClass);

    for (const auto &member: objectData->members()) {
        switch (member.type) {
        case MemberInfo::Type::Property:
            makeProperty(metaObject, member);
            break;

        case MemberInfo::Type::Setter:
        case MemberInfo::Type::Method:
        case MemberInfo::Type::Signal:
        case MemberInfo::Type::Slot:
        case MemberInfo::Type::Constructor:
        case MemberInfo::Type::Invalid:
            break;
        }
    }

    metaObject.setStaticMetacallFunction(metaCallFunction);
    return metaObject.toMetaObject();
}

void MetaObjectBuilder::makeProperty(QMetaObjectBuilder &metaObject,
                                     const MemberInfo   &property)
{
    const auto features = canonical(property.features);
    const auto type     = QMetaType{property.valueType};
    auto metaProperty   = metaObject.addProperty(property.name.value, type.name(), type);

    metaProperty.setReadable  (features & Feature::Read);
    metaProperty.setWritable  (features & Feature::Write);
    metaProperty.setResettable(features & Feature::Reset);
    metaProperty.setDesignable(true);
    metaProperty.setScriptable(true);
    metaProperty.setStored    (true);
    metaProperty.setStdCppSet (features & Feature::Write); // QTBUG-120378
    metaProperty.setFinal     (true);

    if (features & Feature::Notify) {
        auto signature = metaProperty.name() + "Changed(" + type.name() + ")";
        auto metaSignal = metaObject.addSignal(std::move(signature));
        metaSignal.setParameterNames({metaProperty.name()});
        metaProperty.setNotifySignal(std::move(metaSignal));
    } else {
        metaProperty.setConstant(true);
    }
}

} // namespace nproperty::detail
