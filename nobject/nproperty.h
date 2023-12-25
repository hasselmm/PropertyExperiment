#ifndef NPROPERTY_NPROPERTY_H
#define NPROPERTY_NPROPERTY_H

#include "nproperty_p.h"
#include "ntypetraits.h"

#include <QMetaObject>
#include <QMetaType>

namespace nproperty {

/// This macro is needed for each NObject to make it known to Qt's metatype system.
///
#define N_OBJECT                                                                \
                                                                                \
public:                                                                         \
    int qt_metacall(QMetaObject::Call call, int offset, void **args) override   \
    { return staticMetaObject.static_metacall(call, offset, args); }            \
    void *qt_metacast(const char *name) override                                \
    { qCritical("%s(%s)", Q_FUNC_INFO, name); return nullptr; }                 \
    const QMetaObject *metaObject() const override                              \
    { return &staticMetaObject; }                                               \
    static const MetaObject staticMetaObject;                                   \
                                                                                \
private:                                                                        \
    template <quintptr N>                                                       \
    static consteval bool hasMember(detail::Tag<N>) { return false; }           \
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
/// FIXME: consider wording for `detail::LineNumber::current()`
///
#define N_PROPERTY(Type, Name, ...)                                             \
    static consteval bool hasMember(detail::Tag<__LINE__>) { return true; }     \
    static consteval detail::MemberInfo member(detail::Tag<__LINE__>)           \
    { constexpr auto property = &decltype(Name)::ObjectType::Name;              \
      return detail::MemberInfo::make<decltype(Name), property>(); }            \
    Property<Type, name(#Name, __LINE__), ##__VA_ARGS__> Name


/// Theses flags describe various capabilites of a property.
///
enum class Feature
{
    Read    = (1 << 0),
    Reset   = (1 << 1),
    Notify  = (1 << 2),
    Write   = (1 << 3),
};

/// This gives the members of the Feature class a shorter optional
/// name to avoid bloating the class definitions.
///
using enum Feature;

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

    friend ObjectType;

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

    using PublicValue = std::conditional_t<isWritable(), ValueType, std::monostate>;

    /// verbose syntax
    ///
    void resetValue();
    void setValue(PublicValue newValue);
    ValueType value() const { return m_value; }

    /// Qt convenience syntax
    ///
    ValueType operator()() const { return value(); }

    /// Python convenience syntax
    ///
    Property &operator=(PublicValue newValue) { setValue(std::move(newValue)); return *this; }
    operator ValueType() const { return value(); }

    /// Signal emission
    ///
    void notify(Value newValue);

protected:
    using ProtectedValue = std::conditional_t<!isWritable(), ValueType, std::monostate>;

    void setValue(ProtectedValue newValue);
    void setValueImpl(Value &&newValue);

    Property &operator=(ProtectedValue newValue) { setValue(std::move(newValue)); return *this; }

public:
    // Evil address voodoo
    // FIXME: Hide this voodoo. Such evil shall not be public. This all operates on public API
    // and therefore could be free-standing or member functions in the details namespace.
    [[nodiscard]] constexpr quintptr address() const noexcept
    {
        return reinterpret_cast<quintptr>(this);
    }

    [[nodiscard]] static constexpr quintptr offset() noexcept
    {
        return ObjectType::staticMetaObject.memberOffset(Name);
    }

    [[nodiscard]] static constexpr const Property *resolve(const QObject *object) noexcept
    {
        const auto address = reinterpret_cast<quintptr>(object) + offset();
        return reinterpret_cast<const Property *>(address);
    }

    [[nodiscard]] static constexpr Property *resolve(QObject *object) noexcept
    {
        const auto address = reinterpret_cast<quintptr>(object) + offset();
        return reinterpret_cast<Property *>(address);
    }

    [[nodiscard]] constexpr ObjectType *object() const noexcept
    {
        return reinterpret_cast<ObjectType *>(address() - offset());
    }

    [[nodiscard]] detail::MemberFunction<Object, void, Value> notifyPointer() const
    {
        if constexpr (isNotifiable())
            return Object::signalProxy(this);

        return nullptr;
    }

private:
    ValueType m_value;
};

template <class Object, typename Value, auto Name, FeatureSet Features>
inline void Property<Object, Value, Name, Features>::setValue(PublicValue newValue)
{
    static_assert(isWritable());
    static_assert(std::is_same_v<decltype(newValue), Value>);
    setValueImpl(std::move(newValue));
}

template <class Object, typename Value, auto Name, FeatureSet Features>
inline void Property<Object, Value, Name, Features>::setValue(ProtectedValue newValue)
{
    static_assert(std::is_same_v<decltype(newValue), Value>);
    setValueImpl(std::move(newValue));
}

template <class Object, typename Value, auto Name, FeatureSet Features>
inline void Property<Object, Value, Name, Features>::setValueImpl(Value &&newValue)
{
    if constexpr (isNotifiable()) {
        if (std::exchange(m_value, std::move(newValue)) != m_value)
            notify(m_value);
    } else {
        m_value = std::move(newValue);
    }
}

template <class Object, typename Value, auto Name, FeatureSet Features>
inline void Property<Object, Value, Name, Features>::notify(Value newValue)
{
    static_assert(hasFeature(Notify));
    const auto target = object();
    Q_ASSERT(target != nullptr);

    const auto activateSignal = ObjectType::signalProxy(this);
    (target->*activateSignal)(std::move(newValue));
}

} // namespace nproperty

#endif // NPROPERTY_NPROPERTY_H
