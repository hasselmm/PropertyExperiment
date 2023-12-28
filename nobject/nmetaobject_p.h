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
struct MemberInfo
{
    enum class Type {
        Invalid,
        Property,
        Setter,
        Method,
        Signal,
        Slot,
        Constructor,
    };

    using  OffsetFunction =     quintptr(*)();
    using    ReadFunction =         void(*)(const QObject *, void *);
    using   WriteFunction =         void(*)(QObject *, void *);
    using   ResetFunction =         void(*)(QObject *);
    using PointerFunction = const void *(*)();

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
    static consteval MemberInfo make(const char *name) noexcept
    {
        using Object = typename DataMemberType<Property>::ObjectType;

        const auto resolveOffset = [] {
            return reinterpret_cast<quintptr>(Prototype::get(Property))
                   - reinterpret_cast<quintptr>(Prototype::get<Object>());
        };

        const auto selector = Prototype::null(Property);
        return MemberInfo{name, resolveOffset, selector};
    }

    constexpr explicit operator bool() const noexcept { return type != Type::Invalid; }

    Type            type            = Type::Invalid;
    int             valueType       = QMetaType::UnknownType;
    FeatureSet      features;
    LabelId         label;
    const char     *name;

    OffsetFunction  resolveOffset   = nullptr;
    ReadFunction    readProperty    = nullptr;
    WriteFunction   writeProperty   = nullptr;
    ResetFunction   resetProperty   = nullptr;
    PointerFunction pointer         = nullptr;
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

private:
    [[nodiscard]] const MemberInfo *propertyInfo(std::size_t offset) const noexcept;
    [[nodiscard]] const MemberInfo *memberInfo  (std::size_t offset) const noexcept;

    [[nodiscard]] std::function<const MemberInfo *(quintptr)> makeOffsetToSignal() const noexcept;
    [[nodiscard]] int metaMethodForPointer(const void *pointer) const noexcept;

    void readProperty(const QObject *object, std::size_t offset, void *result) const;
    void writeProperty(QObject *object, std::size_t offset, void *value) const;
    void resetProperty(QObject *object, std::size_t offset) const;
    void indexOfMethod(int *result, void *pointer) const noexcept;

private:
    std::vector<MemberInfo>  m_members;

    // these are offsets within `m_members`
    std::vector<std::size_t> m_propertyOffsets;
    std::vector<std::size_t> m_signalOffsets;
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
    static void makeProperty(QMetaObjectBuilder &metaObject,
                             const MemberInfo   &property);
};

} // namespace nproperty::detail

namespace nproperty {

// FIXME: move definition of Property::name() to proper place
template <class Object, typename Value, LabelId Label, FeatureSet Features>
constexpr const char *Property<Object, Value, Label, Features>::name() noexcept
{
    return Object::template member(tag()).name;
}

} // namespace nproperty

#endif // NPROPERTY_NMETAOBJECT_P_H
