#include "nmetaobject.h"

#include <private/qmetaobjectbuilder_p.h>

#include <QObject>
#include <QLoggingCategory>

#include <ranges>

namespace nproperty::detail {

namespace {

Q_LOGGING_CATEGORY(lcMetaObject, "nproperty.metaobject");

namespace ranges {

template<typename T>
concept HasBinarySearch = std::totally_ordered<T>
                          && !std::is_pointer_v<T>
                          && !std::is_floating_point_v<T>;

static_assert(HasBinarySearch<int>);
static_assert(HasBinarySearch<quintptr>);
static_assert(!HasBinarySearch<double>);
static_assert(!HasBinarySearch<const void *>);

template<typename Range, typename T, typename Projection = std::identity>
std::ranges::borrowed_iterator_t<Range> find(Range &&range, const T &value,
                                             Projection projection = {})
{
    if constexpr (std::totally_ordered<T> && !std::is_pointer_v<T>) {
        return std::ranges::lower_bound(range, value, {}, projection);
    } else {
        return std::ranges::find(range, value, projection);
    }
}

int indexOf(auto &&range, const auto &value)
{
    if (const auto it = find(range, value); it != std::cend(range))
        return static_cast<int>(it - std::cbegin(range));

    return -1;
}

} // namespace ranges

quintptr memberToNameLabel(const MemberInfo &member)
{
    return member.name.label;
}

quintptr memberPointerToNameLabel(const MemberInfo *member)
{
    Q_ASSERT(member != nullptr);
    return member->name.label;
}

const void *memberToPointer(const MemberInfo *member)
{
    Q_ASSERT(member != nullptr);
    Q_ASSERT(member->pointer);
    return member->pointer();
}

} // namespace

void MetaObjectData::emplace(MemberInfo &&member)
{
    if (Q_UNLIKELY(!member))
        return;

    if (member.type == MemberInfo::Type::Property) {
        if (canonical(member.features).contains(Feature::Notify))
            m_signalOffsets.emplace_back(m_members.size());

        m_propertyOffsets.emplace_back(m_members.size());
    }

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

quintptr MetaObjectData::memberOffset(quintptr nameIndex) const noexcept
{
    if (const auto it = ranges::find(m_members, nameIndex, memberToNameLabel);
        Q_LIKELY(it != m_members.cend())) {
        Q_ASSERT(it->resolveOffset);
        return it->resolveOffset();
    }

    return 0;
}

std::function<const MemberInfo *(quintptr)> MetaObjectData::makeOffsetToSignal() const noexcept
{
    return [this](quintptr offset) {
        const auto signalInfo = memberInfo(offset);

        Q_ASSERT(signalInfo != nullptr);
        Q_ASSERT(signalInfo->type == MemberInfo::Type::Signal
                 || signalInfo->type == MemberInfo::Type::Property);
        Q_ASSERT(signalInfo->pointer != nullptr);

        return signalInfo;
    };
}

int MetaObjectData::metaMethodForPointer(const void *pointer) const noexcept
{
    return ranges::indexOf(m_signalOffsets
                               | std::views::transform(makeOffsetToSignal())
                               | std::views::transform(memberToPointer),
                           pointer);
}

int MetaObjectData::metaMethodForName(quintptr nameIndex) const noexcept
{
    return ranges::indexOf(m_signalOffsets
                               | std::views::transform(makeOffsetToSignal())
                               | std::views::transform(memberPointerToNameLabel),
                           nameIndex);
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

void MetaObjectData::indexOfMethod(int *result, void *pointer) const noexcept
{
    *result = metaMethodForPointer(pointer);
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
