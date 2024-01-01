#ifndef NPROPERTY_NMETAOBJECT_P_H
#define NPROPERTY_NMETAOBJECT_P_H

#include "nconcepts.h"
#include "nmetaenum.h"
#include "nproperty.h"
#include "ntypetraits.h"

#include <QMetaObject>
#include <QMetaType>

class QMetaObjectBuilder;

namespace nproperty::detail {

using metaenum::KeyInfoArray;

/// Introspection information about class members.
///
struct MemberInfo // FIXME: actually this is ObjectInfo, MetaInfo, or the like...
{
    enum class Type {
        Invalid,
        Interface,
        ClassInfo,
        Property,
        Signal,
        InlineEnum,
        ScopedEnum,
        InlineFlag,
        ScopedFlag,
    };

    using  OffsetFunction =     quintptr(*)();
    using    ReadFunction =         void(*)(const QObject *, void *);
    using   WriteFunction =         void(*)(QObject *, void *);
    using   ResetFunction =         void(*)(QObject *);
    using PointerFunction = const void *(*)();
    using    CastFunction =       void *(*)(QObject *);
    using KeyInfoFunction = KeyInfoArray(*)();

    consteval MemberInfo() noexcept = default;

    template <class Object, typename Value, LabelId Label, FeatureSet Features>
    consteval MemberInfo(std::string_view        name,
                         OffsetFunction resolveOffset,
                         const Property<Object, Value, Label, Features> * = nullptr) noexcept
        : type{Type::Property}
        , valueType{qMetaTypeId<Value>()}
        , features{Features}
        , label{Label}
        , name{name}
        , resolveOffset{resolveOffset}
        , readProperty{[](const QObject *object, void *result) {
            const auto property = Property<Object, Value, Label, Features>::resolve(object);
            *reinterpret_cast<Value *>(result) = property->value();
        }}
        , writeProperty{[](QObject *object, void *value) {
            if constexpr (canonical(Features).contains(Feature::Write)) {
                const auto property = Property<Object, Value, Label, Features>::resolve(object);
                property->setValue(*reinterpret_cast<Value *>(value));
            }
        }}
        , resetProperty{[](QObject *object) {
            if constexpr (canonical(Features).contains(Feature::Reset)) {
                const auto property = Property<Object, Value, Label, Features>::resolve(object);
                property->resetValue();
            }
        }}
        , pointer{[] {
            const auto proxy = Object::template signalProxy<Value, Label, Features>();
            return *reinterpret_cast<const void *const *>(&proxy);
        }}
    {}

    static constexpr bool isEnumOrFlag(Type type) noexcept
    {
        switch (type) {
        case Type::InlineEnum:
        case Type::ScopedEnum:
        case Type::InlineFlag:
        case Type::ScopedFlag:
            return true;

        case Type::Invalid:
        case Type::Interface:
        case Type::ClassInfo:
        case Type::Property:
        case Type::Signal:
            break;
        }

        return false;
    }

    static constexpr bool isFlag(Type type) noexcept
    {
        switch (type) {
        case Type::InlineFlag:
        case Type::ScopedFlag:
            return true;

        case Type::Invalid:
        case Type::Interface:
        case Type::ClassInfo:
        case Type::Property:
        case Type::Signal:
        case Type::InlineEnum:
        case Type::ScopedEnum:
            break;
        }

        return false;
    }

    static constexpr bool isScoped(Type type) noexcept
    {
        switch (type) {
        case Type::ScopedEnum:
        case Type::ScopedFlag:
            return true;

        case Type::Invalid:
        case Type::Interface:
        case Type::ClassInfo:
        case Type::Property:
        case Type::Signal:
        case Type::InlineEnum:
        case Type::InlineFlag:
            break;
        }

        return false;
    }

    template<EnumType T>
    static constexpr Type enumType() noexcept
    {
        if constexpr (ScopedEnumType<T>) {
            return Type::ScopedEnum;
        } else {
            return Type::InlineEnum;
        }
    }

    template<EnumType T>
    static constexpr Type flagType() noexcept
    {
        if constexpr (ScopedEnumType<T>) {
            return Type::ScopedFlag;
        } else {
            return Type::InlineFlag;
        }
    }

    constexpr bool isFlag()   const noexcept { return isFlag  (type); }
    constexpr bool isScoped() const noexcept { return isScoped(type); }

    template<auto Property>
    static consteval MemberInfo makeProperty(std::string_view name) noexcept
    {
        using Object = typename DataMemberType<Property>::ObjectType;

        const auto resolveOffset = [] {
            return reinterpret_cast<quintptr>(Prototype::get(Property))
                   - reinterpret_cast<quintptr>(Prototype::get<Object>());
        };

        const auto selector = Prototype::null(Property);
        return MemberInfo{std::move(name), resolveOffset, selector};
    }

    static consteval MemberInfo makeClassInfo(LabelId          label,
                                              std::string_view name,
                                              std::string_view value) noexcept
    {
        auto classInfo  = MemberInfo{};
        classInfo.type  = MemberInfo::Type::ClassInfo;
        classInfo.label = label;
        classInfo.name  = std::move(name);
        classInfo.value = std::move(value);
        return classInfo;
    }

    template<QtInterface Interface, std::derived_from<QObject> Object>
    static constexpr MemberInfo makeInterface() noexcept
    {
        auto interfaceInfo     = MemberInfo{};
        interfaceInfo.type     = Type::Interface;
        interfaceInfo.name     = QMetaType::fromType<Interface>().name();
        interfaceInfo.value    = qobject_interface_iid<Interface *>();
        interfaceInfo.metacast = [](QObject *object) {
            const auto self = static_cast<Object *>(object);
            return static_cast<void *>(static_cast<Interface *>(self));
        };

        return interfaceInfo;
    }

    template<EnumType Enum, MemberInfo::Type type>
    requires(isEnumOrFlag(type))
    static constexpr MemberInfo makeEnumerator(LabelId label) noexcept
    {
        auto enumeratorInfo  = MemberInfo{};
        enumeratorInfo.type  = type;
        enumeratorInfo.label = label;
        enumeratorInfo.name  = metaenum::name<Enum>(); // QMetaType::fromType<Enum>() is not constexpr
        enumeratorInfo.keys  = metaenum::keys<Enum, isFlag(type)>;

        return enumeratorInfo;
    }

    constexpr explicit operator bool() const noexcept { return type != Type::Invalid; }

    Type             type           = Type::Invalid;
    int              valueType      = QMetaType::UnknownType;
    FeatureSet       features       = {};
    LabelId          label          = 0;
    std::string_view name;
    std::string_view value;

    OffsetFunction   resolveOffset  = nullptr;
    ReadFunction     readProperty   = nullptr;
    WriteFunction    writeProperty  = nullptr;
    ResetFunction    resetProperty  = nullptr;
    PointerFunction  pointer        = nullptr;
    CastFunction     metacast       = nullptr;
    KeyInfoFunction  keys           = nullptr;
};

/// Introspection information about a C++ class that can be used to build a `QMetaObject`.
///
class MetaObjectData
{
public:
    [[nodiscard]] const auto &members() const noexcept { return m_members; }
    [[nodiscard]] quintptr memberOffset(LabelId label) const noexcept;
    [[nodiscard]] int metaMethodIndexForLabel(LabelId label) const noexcept;

protected:
    void emplace(MemberInfo &&member);
    void metaCall(QObject *object, QMetaObject::Call call, int offset, void **args) const;
    void *interfaceCast(QObject *object, std::string_view name) const;

    template<class Object, quintptr N>
    static consteval bool hasMember()
    {
        return std::is_same_v<MemberInfo, decltype(Object::member(Tag<N>{}))>;
    }

    void validateMembers() const;

private:
    using MemberTable  = std::vector<MemberInfo>;
    using MemberOffset = MemberTable::size_type;

    [[nodiscard]] const MemberInfo *propertyInfo(MemberOffset offset) const noexcept;
    [[nodiscard]] const MemberInfo *memberInfo  (MemberOffset offset) const noexcept;

    [[nodiscard]] std::function<const MemberInfo *(quintptr)> makeOffsetToInterface() const noexcept;
    [[nodiscard]] std::function<const MemberInfo *(quintptr)> makeOffsetToSignal() const noexcept;
    [[nodiscard]] int metaMethodForPointer(const void *pointer) const noexcept;

    void readProperty(const QObject *object, MemberOffset offset, void *result) const;
    void writeProperty(QObject *object, MemberOffset offset, void *value) const;
    void resetProperty(QObject *object, MemberOffset offset) const;
    void indexOfMethod(int *result, void *pointer) const noexcept;

private:
    MemberTable m_members;

    std::vector<MemberOffset> m_interfaceOffsets;
    std::vector<MemberOffset> m_propertyOffsets;
    std::vector<MemberOffset> m_signalOffsets;
};

/// Builds a QMetaObject from our static introspection information.
///
class MetaObjectBuilder
{
public:
    using MetaCallFunction = void (*)(QObject *, QMetaObject::Call, int, void **);

    [[nodiscard]] static const QMetaObject *build(const QMetaType          &metaType,
                                                  const QMetaObject      *superClass,
                                                  const MetaObjectData   *objectData,
                                                  MetaCallFunction  metaCallFunction);

private:
    static void makeProperty  (QMetaObjectBuilder &metaObject, const MemberInfo &property);
    static void makeClassInfo (QMetaObjectBuilder &metaObject, const MemberInfo &classInfo);
    static void makeEnumerator(QMetaObjectBuilder &metaObject, const MemberInfo &enumeratorInfo);
};

} // namespace nproperty::detail

namespace nproperty {

// FIXME: move definition of Property::name() to proper place
// turning Object::member() into a free-standing function might do the trick (#24)
template <class Object, typename Value, LabelId Label, FeatureSet Features>
constexpr std::string_view Property<Object, Value, Label, Features>::name() noexcept
{
    // HACK: This lambda is a workaround for MSVC wrongly reporting C7595:
    // 'npropertytest::HelloWorld::member': call to immediate function is not a constant expression
    constexpr auto name = [](const auto &member) {
        return member.name;
    };

    return name(Object::member(tag()));
}

} // namespace nproperty

#endif // NPROPERTY_NMETAOBJECT_P_H
