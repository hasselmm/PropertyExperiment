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

    using OffsetFunction = quintptr(*)();
    using   ReadFunction =     void(*)(const QObject *, void *);
    using  WriteFunction =     void(*)(QObject *, void *);
    using  ResetFunction =     void(*)(QObject *);

    consteval MemberInfo() noexcept = default;

    template<class Property, auto Member>
    static consteval MemberInfo make()
    {
        const auto selector = static_cast<const Property *>(nullptr);

        auto resolveOffset = [] {
            using ObjectType = typename Property::ObjectType;

            return reinterpret_cast<quintptr>(Prototype::get(Member))
                   - reinterpret_cast<quintptr>(Prototype::get<ObjectType>());
        };

        return MemberInfo{selector, std::move(resolveOffset)};
    }

    template <class Object, typename Value, auto Name, FeatureSet Features>
    consteval MemberInfo(const Property<Object, Value, Name, Features> *,
                         OffsetFunction resolveOffset) noexcept
        : type{Type::Property}
        , valueType{qMetaTypeId<Value>()}
        , features{Features}
        , name{Name.index, Name.value}
        , resolveOffset{resolveOffset}
        , readProperty{[](const QObject *object, void *result) {
            const auto property = Property<Object, Value, Name, Features>::resolve(object);
            *reinterpret_cast<Value *>(result) = property->value();
        }}
        , writeProperty{[](QObject *object, void *value) {
            const auto property = Property<Object, Value, Name, Features>::resolve(object);
            property->setValue(*reinterpret_cast<Value *>(value));
        }}
        , resetProperty{[](QObject *object) {
            const auto property = Property<Object, Value, Name, Features>::resolve(object);
            property->resetValue();
        }}
    {}

    constexpr explicit operator bool() const noexcept { return type != Type::Invalid; }

    struct Name {
        quintptr    index           = 0;
        const char *value           = nullptr;
    };

    Type            type            = Type::Invalid;
    int             valueType       = QMetaType::UnknownType;
    FeatureSet      features;
    Name            name;

    OffsetFunction  resolveOffset   = nullptr;
    ReadFunction    readProperty    = nullptr;
    WriteFunction   writeProperty   = nullptr;
    ResetFunction   resetProperty   = nullptr;
};

/// Introspection information about a C++ class that can be used to build a `QMetaObject`.
///
class MetaObjectData
{
public:
    [[nodiscard]] const std::vector<MemberInfo> &members() const noexcept
    { return m_members; }

    template<quintptr N>
    [[nodiscard]] quintptr memberOffset(Name<N> name) const noexcept
    { return memberOffset(name.index); }

protected:
    void emplace(MemberInfo &&member);
    void metaCall(QObject *object, QMetaObject::Call call, int offset, void **args) const;

private:
    [[nodiscard]] const MemberInfo *propertyInfo(std::size_t offset) const noexcept;
    [[nodiscard]] const MemberInfo *memberInfo  (std::size_t offset) const noexcept;

    [[nodiscard]] quintptr memberOffset(quintptr name) const noexcept;

    void readProperty(const QObject *object, std::size_t offset, void *result) const;
    void writeProperty(QObject *object, std::size_t offset, void *value) const;
    void resetProperty(QObject *object, std::size_t offset) const;
    void indexOfMethod(int *result, void *pointer) const noexcept;

private:
    std::vector<MemberInfo>  m_members;

    // these are offsets within `m_members`
    std::vector<std::size_t> m_propertyOffsets;
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

#endif // NPROPERTY_NMETAOBJECT_P_H
