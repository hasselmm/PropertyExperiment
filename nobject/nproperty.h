#ifndef NPROPERTY_NPROPERTY_H
#define NPROPERTY_NPROPERTY_H

#include "nproperty_p.h"
#include "qtypes.h"

#include <QMetaObject>
#include <QMetaType>

#include <source_location>

namespace nproperty {

/// This macro is needed for each NObject to make it known to Qt's metatype system.
///
#define N_OBJECT                                                                \
public:                                                                         \
    int qt_metacall(QMetaObject::Call call, int offset, void **args)            \
    { return staticMetaObject.static_metacall(call, offset, args); }            \
                                                                                \
    static const MetaObject staticMetaObject;                                   \
                                                                                \
private:                                                                        \
    template <quintptr N>                                                       \
    static consteval detail::MemberInfo member(detail::Tag<N>) { return {}; }   \
    static consteval quintptr lineOffset() { return __LINE__; }                 \
    friend MetaObject;                                                          \
    friend Object;


/// This macro just defines the static metaobject of this class. This cannot
/// be done in the `N_OBJECT` macro, as C++ only allows initialization of static
/// members in the class definition, if these members are constexpr. Such is
/// not possible as the static meta object must be an instance of QMetaObject.
///
#define N_OBJECT_IMPLEMENTATION(ClassName)                                      \
const ClassName::MetaObject ClassName::staticMetaObject = {};

/// This macro provides convenience and pretty syntax when adding new properties
/// to a NObject.
///
/// ``` C++
/// N_PROPERTY<QString, nice> = "Hello World";
/// ```
///
/// The alternate syntax would be, to explicitly invoke the name()
/// function, and to write the property name twice.
///
/// For older compilers without functional std::source_location one would have
/// additionally to pass a numeric identifier unique to the current class,
/// e.g. `__LINE__`.
///
/// ``` C++
/// Property<QString, name("modern")> modern;
/// Property<QString, name("legacy", __LINE__)> legacy;
/// ```
///
#define N_PROPERTY(Type, Name, ...)                                             \
static consteval detail::MemberInfo member(detail::Tag<__LINE__>)               \
{ return {static_cast<decltype(Name) *>(nullptr)}; }                            \
Property<Type, name(#Name), ##__VA_ARGS__> Name

/// Theses flags describe various capabilites of a property.
///
enum class Feature
{
    Read    = (1 << 0),
    Reset   = (1 << 1),
    Notify  = (1 << 2),
    Write   = (1 << 3),
};

using FeatureSet = detail::Flags<Feature>;

constexpr FeatureSet operator|(Feature lhs, Feature rhs) noexcept
{ return FeatureSet{lhs} | rhs; }

/// Some features only make sense, or only can be reasonably
/// implemented in combination with other features. This function
/// is used to add theses incomplete rules to a feature set.
///
static constexpr FeatureSet canonical(FeatureSet features)
{
    if (features &  Feature::Write)
        features |= Feature::Notify;
    if (features &  Feature::Reset)
        features |= Feature::Notify;
    if (features &  Feature::Notify)
        features |= Feature::Read;

    return features;
}

/// A QObject property that's fully defined in pure C++.
///
/// Template arguments:
/// * `Object` - the type of the QObject class to wich this property is added
/// * `Value` - the actual value type of this property
/// * `Name` - the unique name of this property
///
template <class Object, typename Value, auto Name, FeatureSet Features = Feature::Read>
class Property
{
public:
    using ObjectType = Object;
    using  ValueType = Value;

    Property() noexcept = default;
    Property(ValueType value) noexcept
        : m_value{std::move(value)}
    {}

    [[nodiscard]] static constexpr quintptr index() noexcept            { return Name.index; }
    [[nodiscard]] static constexpr const char *name() noexcept          { return Name.value; }

    [[nodiscard]] static constexpr FeatureSet features() noexcept       { return canonical(Features); }
    [[nodiscard]] static constexpr bool hasFeature(Feature f) noexcept  { return features().contains(f); }

    [[nodiscard]] static constexpr bool isReadable() noexcept           { return hasFeature(Feature::Read); }
    [[nodiscard]] static constexpr bool isResetable() noexcept          { return hasFeature(Feature::Reset); }
    [[nodiscard]] static constexpr bool isNotifiable() noexcept         { return hasFeature(Feature::Notify); }
    [[nodiscard]] static constexpr bool isWritable() noexcept           { return hasFeature(Feature::Write); }

    /// verbose syntax
    void resetValue() { setValue({}); }
    void setValue(ValueType newValue);
    ValueType value() const { return m_value; }

    /// Qt convenience syntax
    ValueType operator()() const { return value(); }

    /// Python convenience syntax
    Property &operator=(Value newValue);
    operator ValueType() const { return value(); }

private:
    ValueType m_value;
};


namespace detail {

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

    constexpr MemberInfo() noexcept = default;

    template <class Object, typename Value, auto Name, FeatureSet Features>
    constexpr MemberInfo(const Property<Object, Value, Name, Features> *) noexcept
        : type{Type::Property}
        , index{Name.index}
        , name{Name.value}
        , valueType{qMetaTypeId<Value>()}
        , features{Features}
    {}

    constexpr explicit operator bool() const noexcept { return type != Type::Invalid; }

    Type        type        = Type::Invalid;
    quintptr    index       = 0;
    const char *name        = nullptr;

    int         valueType   = QMetaType::UnknownType;
    FeatureSet  features;
};

} // namespace detail

/// This class provides the introspection information for this `ObjectType`.
///
/// Template arguments:
/// * `ObjectType` - the type of the object to describe
/// * `SuperType` - the super class of `ObjectType`
/// * `MaximumLineCount` - the maximum number of lines in the class definition,
///                        simply sizeof(ObjectType) if zero gets passed
///
template<class ObjectType, class SuperType, std::size_t MaximumLineCount = 0>
class MetaObject : public QMetaObject
{
public:
    MetaObject() : QMetaObject{build()} {}

private:
    QMetaObject build()
    {
        constexpr auto lineCount = std::max(sizeof(ObjectType), MaximumLineCount);
        constexpr auto lines = std::make_integer_sequence<quintptr, lineCount>();
        registerMembers<ObjectType::lineOffset()>(lines);

        auto builder = detail::MetaObjectBuilder{};
        return builder.makeMetaObject(QMetaType::fromType<ObjectType>(),
                                      &SuperType::staticMetaObject,
                                      m_members);
    }

    template<quintptr Offset, quintptr... Indices>
    Q_ALWAYS_INLINE void registerMembers(const std::integer_sequence<quintptr, Indices...> &)
    {
        (registerMember<Offset + Indices>(), ...);
    }

    template<quintptr Index>
    Q_ALWAYS_INLINE void registerMember()
    {
        if (constexpr auto member = ObjectType::template member(detail::Tag<Index>{}))
            m_members.emplace_back(std::move(member));
    }

    std::vector<detail::MemberInfo> m_members;
};

/// This mixin provides convenience methods for classes implementing
/// this property system. Most likely all them then can mix merged
/// into the N_OBJECT macro, and this intermediate class can be
/// removed.
///
/// FIXME: Check if this class can be removed.
///
template <class ObjectType, class SuperType = QObject>
class Object : public SuperType
{
    friend nproperty::MetaObject<ObjectType, SuperType>;

public:
    using MetaObject = nproperty::MetaObject<ObjectType, SuperType>;

    using SuperType::SuperType;

protected:
    template <typename Value, auto Name, FeatureSet Features = Feature::Read>
    using Property = nproperty::Property<ObjectType, Value, Name, Features>;

    template <std::size_t N>
    static consteval detail::Name<N> name(const char (&name)[N],
                                          std::source_location source =
                                          std::source_location::current())
    {
        return detail::Name{name, std::move(source).line()};
    }

    template <std::size_t N>
    static consteval detail::Name<N> name(quintptr index, const char (&name)[N])
    {
        return detail::Name{name, index};
    }

    template <quintptr N>
    static consteval const char *name(detail::Tag<N>) { return nullptr; }
};

/// This gives the members of the Feature class a shorter optional
/// name to avoid bloating the class definitions. It's defined at
/// the very end of this header to void ambiguities in this header.
///
using enum Feature;

} // namespace nproperty

#endif // NPROPERTY_NPROPERTY_H
