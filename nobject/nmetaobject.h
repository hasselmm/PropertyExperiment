#ifndef NPROPERTY_NMETAOBJECT_H
#define NPROPERTY_NMETAOBJECT_H

#include "nmetaobject_p.h"
#include "nlinenumber_p.h"

namespace nproperty {

template<class ObjectType, class SuperType, QtInterface... Interfaces>
class Object;

template<class T>
constexpr std::size_t MaximumLineCount = 0;

/// This class provides the introspection information for this `ObjectType`.
///
/// Template arguments:
/// * `ObjectType`       - the type of the object to describe
/// * `SuperType`        - the super class of `ObjectType`
///
template<class ObjectType, class SuperType, QtInterface... Interfaces>
class MetaObject
    : public detail::MetaObjectData // FIXME: make private or protected
    , public QMetaObject
{
    friend Object<ObjectType, SuperType>;
    friend ObjectType;

public:
    MetaObject()
        : MetaObjectData{}
        , QMetaObject{*build()} // FIXME: Is this a leak, or not?
    {}

    void *metaCast(ObjectType *object, const char *name) const
    {
        if (name == nullptr)
            return nullptr; // guard strcmp() from segfault
        if (std::strcmp(name, className()) == 0)
            return object;
        if (const auto result = interfaceCast(object, name))
            return result;

        return object->SuperType::qt_metacast(name);
    }

private:
    const QMetaObject *build()
    {
        static const auto s_metaObject = [](MetaObject *data) {
            constexpr auto lineCount = std::max(sizeof(ObjectType), MaximumLineCount<ObjectType>);
            constexpr auto lines = std::make_integer_sequence<LabelId, lineCount>();
            registerMembers<ObjectType::lineOffset()>(data, lines);
            data->validateMembers();

            return detail::MetaObjectBuilder::build(QMetaType::fromType<ObjectType>(),
                                                    &SuperType::staticMetaObject,
                                                    &ObjectType::staticMetaObject,
                                                    &MetaObject::staticMetaCall);
        }(this);

        return s_metaObject;
    }

    template<LabelId... Indices>
    using LabelSequence = std::integer_sequence<LabelId, Indices...>;

    template<quintptr Offset, LabelId... Labels>
    static void registerMembers(MetaObject *data, const LabelSequence<Labels...> &)
    {
        (registerInterface<Interfaces>(data), ...);
        (registerMember<Offset + Labels>(data), ...);
    }

    template<LabelId Label>
    static void registerMember(MetaObject *data)
    {
        // the constexpr is essential here to avoid generating huge amount of code
        if constexpr (hasMember<ObjectType, Label>())
            data->emplace(ObjectType::member(detail::Tag<Label>{}));
    }

    template<typename Interface>
    static void registerInterface(MetaObject *data)
    {
        data->emplace(detail::MemberInfo::makeInterface<Interface, ObjectType>());
    }

    static void staticMetaCall(QObject *object, QMetaObject::Call call, int offset, void **args)
    {
        ObjectType::staticMetaObject.metaCall(object, call, offset, args);
    }
};

/// This mixin provides convenience methods for classes implementing
/// this property system. Most likely all them then can mix merged
/// into the N_OBJECT macro, and this intermediate class can be
/// removed.
///
/// FIXME: Check if this class can be removed.
///
template <class ObjectType, class SuperType = QObject, QtInterface... Interfaces>
class Object
    : public SuperType
    , public Interfaces...
{
    friend nproperty::MetaObject<ObjectType, SuperType, Interfaces...>;
    friend nproperty::detail::MemberInfo;

public:
    using MetaObject = nproperty::MetaObject<ObjectType, SuperType, Interfaces...>;
    using TargetType = ObjectType;

    using SuperType::SuperType;

protected:
    template <typename Value, auto Name, FeatureSet Features = Feature::Read>
    using Property = nproperty::Property<ObjectType, Value, Name, Features>;

    /// Generate a unique `LabelId` form current line number.
    static consteval LabelId l(LabelId l = detail::LineNumber::current()) noexcept { return l; }

    // The label is part of the method signature
    // as we need unique method pointers for QObject::connect().
    template<typename Value, LabelId Label>
    void activateSignal(Value value)
    {
        const auto metaObject = &ObjectType::staticMetaObject;
        const auto methodIndex = metaObject->metaMethodIndexForLabel(Label);
              auto metaCallArgs = std::array<void *, 2> { nullptr, &value };

        QMetaObject::activate(this, metaObject, methodIndex, metaCallArgs.data());
    }

    template<auto Property>
    static consteval auto makeProperty(std::string_view name) noexcept
    {
        return detail::MemberInfo::makeProperty<Property>(std::move(name));
    }

    static consteval auto makeClassInfo(std::string_view name, std::string_view value,
                                        LabelId label = detail::LineNumber::current()) noexcept
    {
        return detail::MemberInfo::makeClassInfo(label, std::move(name), std::move(value));
    }

    template<EnumType T>
    static consteval auto makeEnum(LabelId label = detail::LineNumber::current())
    {
        constexpr auto type = detail::MemberInfo::enumType<T>();
        return detail::MemberInfo::makeEnumerator<T, type>(label);
    }

    template<EnumType T>
    static consteval auto makeFlag(LabelId label = detail::LineNumber::current())
    {
        constexpr auto type = detail::MemberInfo::flagType<T>();
        return detail::MemberInfo::makeEnumerator<T, type>(label);
    }

    template <LabelId N>
    static constexpr bool hasMember() noexcept
    {
        return MetaObject::template hasMember<ObjectType, N>();
    }

public: // FIXME: make signalProxy() protected again
    template<typename Value, LabelId Label, FeatureSet Features>
    static detail::MemberFunction<ObjectType, void, Value>
    signalProxy(const Property<Value, Label, Features> * = nullptr)
    {
        if (Q_LIKELY(canonical(Features).contains(Feature::Notify)))
            return &ObjectType::template activateSignal<Value, Label>;

        return nullptr;
    }
};

/// Alias for a properties change notification signal.
/// Use it to simulate the established Qt code style.
///
/// The `ObjectType` argument is simply a work around for Clang 16 wrongly
/// resolving `Property` in the N_PROPERTY_NOTIFICATION() macro. You can
/// usually ignore it.
///
template<auto Property, typename ObjectType = void>
requires(detail::DataMemberType<Property>::isNotifiable())
class Signal
{
public:
    constexpr auto get() const noexcept { return detail::Prototype::get(Property)->notifyPointer(); }
    constexpr auto operator&() const noexcept { return get(); }
};

} // namespace nproperty

#endif // NPROPERTY_NMETAOBJECT_H
