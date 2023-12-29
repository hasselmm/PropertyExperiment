#ifndef NPROPERTY_NMETAOBJECT_P_H
#define NPROPERTY_NMETAOBJECT_P_H

#include "nproperty.h"
#include "ntypetraits.h"

#include <QMetaObject>
#include <QMetaType>

class QMetaObjectBuilder;

namespace nproperty::detail {

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
    };

    using  OffsetFunction =     quintptr(*)();
    using    ReadFunction =         void(*)(const QObject *, void *);
    using   WriteFunction =         void(*)(QObject *, void *);
    using   ResetFunction =         void(*)(QObject *);
    using PointerFunction = const void *(*)();
    using    CastFunction =       void *(*)(QObject *);

    consteval MemberInfo() noexcept = default;

    template <class Object, typename Value, LabelId Label, FeatureSet Features>
    consteval MemberInfo(const char *name,
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

    template<auto Property>
    static consteval MemberInfo makeProperty(const char *name) noexcept
    {
        using Object = typename DataMemberType<Property>::ObjectType;

        const auto resolveOffset = [] {
            return reinterpret_cast<quintptr>(Prototype::get(Property))
                   - reinterpret_cast<quintptr>(Prototype::get<Object>());
        };

        const auto selector = Prototype::null(Property);
        return MemberInfo{name, resolveOffset, selector};
    }

<<<<<<< HEAD
    static consteval MemberInfo makeClassInfo(LabelId     label,
                                              const char *name,
=======
    static consteval MemberInfo makeClassInfo(const char *name,
>>>>>>> 2463cef (Allow to store interface information in MemberInfo)
                                              const char *value) noexcept
    {
        auto classInfo  = MemberInfo{};
        classInfo.type  = MemberInfo::Type::ClassInfo;
        classInfo.label = label;
        classInfo.name  = name;
        classInfo.value = value;
        return classInfo;
    }

    static constexpr MemberInfo makeInterface(const char  *name,
                                              const char   *iid,
                                              CastFunction cast) noexcept
    {
        auto interfaceInfo     = MemberInfo{};
        interfaceInfo.type     = MemberInfo::Type::Interface;
        interfaceInfo.name     = name;
        interfaceInfo.value    = iid;
        interfaceInfo.metacast = cast;
        return interfaceInfo;
    }

    constexpr explicit operator bool() const noexcept { return type != Type::Invalid; }

    Type            type            = Type::Invalid;
    int             valueType       = QMetaType::UnknownType;
    FeatureSet      features        = {};
    LabelId         label           = 0;
    const char     *name            = nullptr;
    const char     *value           = nullptr;

    OffsetFunction  resolveOffset   = nullptr;
    ReadFunction    readProperty    = nullptr;
    WriteFunction   writeProperty   = nullptr;
    ResetFunction   resetProperty   = nullptr;
    PointerFunction pointer         = nullptr;
    CastFunction    metacast        = nullptr;
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
    static void makeProperty (QMetaObjectBuilder &metaObject, const MemberInfo &property);
    static void makeClassInfo(QMetaObjectBuilder &metaObject, const MemberInfo &classInfo);
};

} // namespace nproperty::detail

namespace nproperty {

// FIXME: move definition of Property::name() to proper place
template <class Object, typename Value, LabelId Label, FeatureSet Features>
constexpr const char *Property<Object, Value, Label, Features>::name() noexcept
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
