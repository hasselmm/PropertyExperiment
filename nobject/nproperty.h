#ifndef NPROPERTY_NPROPERTY_H
#define NPROPERTY_NPROPERTY_H

#include "nproperty_p.h"
#include "ntypetraits.h"

#include <QObject>

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
protected:                                                                      \
    template <quintptr N>                                                       \
    static consteval void member(::nproperty::detail::Tag<N>) {}                \
                                                                                \
private:                                                                        \
    static consteval quintptr lineOffset() { return __LINE__; }                 \
    friend ::nproperty::detail::MetaObjectData;                                 \
    friend MetaObject;                                                          \
    friend Object;

/// This macro just defines the static metaobject of this class. This cannot
/// be done in the `N_OBJECT` macro, as C++ only allows initialization of static
/// members in the class definition, if these members are constexpr. Such is
/// not possible as the static meta object must be an instance of QMetaObject.
///
#define N_OBJECT_IMPLEMENTATION(ClassName) \
const ClassName::MetaObject ClassName::staticMetaObject = {};


/// Register a property for introspection.
/// This is mainly for special cases. You might prefer N_PROPERTY().
///
#define N_REGISTER_PROPERTY(PropertyName) \
    static consteval auto member(::nproperty::detail::Tag<__LINE__>) \
    { return makeMember<&TargetType::PropertyName>(#PropertyName); }

/// Defines a class member for connecting to property notifications using
/// traditional syntax: `connect(object, &Object::propertyChanged, ...);`
///
#define N_PROPERTY_NOTIFICATION(PropertyName) \
    static constexpr ::nproperty::Signal<&TargetType::PropertyName, \
                                          TargetType> PropertyName ## Changed = {};

/// Defines a class member for chaging properties using traditional syntax:
/// `object.setProperty(newValue);`
///
#define N_PROPERTY_SETTER(SetterName, PropertyName) \
    void SetterName(decltype(TargetType::PropertyName)::ValueType newValue) \
    { PropertyName.setValue(std::move(newValue)); }


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
#define N_PROPERTY(Type, Name, ...) \
    N_REGISTER_PROPERTY(Name) \
    Property<Type, __LINE__, ##__VA_ARGS__> Name


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

/// A unique number identifying members within their object, usually just the line number.
///
using LabelId = quintptr;

/// A QObject property that's fully defined in pure C++.
///
/// Template arguments:
/// * `Object` - the type of the QObject class to wich this property is added
/// * `Value` - the actual value type of this property
/// * `Label` - the unique number identifying this property within its object
///
template <class Object, typename Value, LabelId Label, FeatureSet Features = Feature::Read>
class Property
{
public:
    using ObjectType = Object;
    using  ValueType = Value;
    using    TagType = detail::Tag<Label>;

    friend ObjectType;

    Property() noexcept = default;
    Property(ValueType value) noexcept
        : m_value{std::move(value)}
    {}

    [[nodiscard]] static constexpr TagType tag() noexcept               { return {}; }
    [[nodiscard]] static constexpr LabelId label() noexcept             { return Label; }
    [[nodiscard]] static constexpr const char *name() noexcept;

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

    template<std::derived_from<QObject> Receiver, typename Functor>
    QMetaObject::Connection connect(Receiver *context, Functor functor) const
    {
        return QObject::connect(object(), notifyPointer(), context, std::move(functor));
    }

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
        return ObjectType::staticMetaObject.memberOffset(label());
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

template <class Object, typename Value, LabelId Label, FeatureSet Features>
inline void Property<Object, Value, Label, Features>::setValue(PublicValue newValue)
{
    static_assert(isWritable());
    static_assert(std::is_same_v<decltype(newValue), Value>);
    setValueImpl(std::move(newValue));
}

template <class Object, typename Value, LabelId Label, FeatureSet Features>
inline void Property<Object, Value, Label, Features>::setValue(ProtectedValue newValue)
{
    static_assert(std::is_same_v<decltype(newValue), Value>);
    setValueImpl(std::move(newValue));
}

template <class Object, typename Value, LabelId Label, FeatureSet Features>
inline void Property<Object, Value, Label, Features>::setValueImpl(Value &&newValue)
{
    if constexpr (isNotifiable()) {
        if (std::exchange(m_value, std::move(newValue)) != m_value)
            notify(m_value);
    } else {
        m_value = std::move(newValue);
    }
}

template <class Object, typename Value, LabelId Label, FeatureSet Features>
inline void Property<Object, Value, Label, Features>::notify(Value newValue)
{
    static_assert(isNotifiable());
    const auto target = object();
    Q_ASSERT(target != nullptr);

    const auto activateSignal = ObjectType::signalProxy(this);
    (target->*activateSignal)(std::move(newValue));
}

} // namespace nproperty

#endif // NPROPERTY_NPROPERTY_H
