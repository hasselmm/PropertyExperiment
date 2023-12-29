#ifndef NPROPERTY_NMETAOBJECT_H
#define NPROPERTY_NMETAOBJECT_H

#include "nmetaobject_p.h"
#include "nlinenumber_p.h"

namespace nproperty {

template<class ObjectType, class SuperType>
class Object;

/// This class provides the introspection information for this `ObjectType`.
///
/// Template arguments:
/// * `ObjectType`       - the type of the object to describe
/// * `SuperType`        - the super class of `ObjectType`
/// * `MaximumLineCount` - the maximum number of lines in the class definition,
///                        simply sizeof(ObjectType) if zero gets passed
///
template<class ObjectType, class SuperType, std::size_t MaximumLineCount = 0>
class MetaObject
    : public detail::MetaObjectData // FIXME: make private or protected
    , public QMetaObject
{
    friend Object<ObjectType, SuperType>;

public:
    MetaObject()
        : MetaObjectData{}
        , QMetaObject{*build()} // FIXME: Is this a leak, or not?
    {}

private:
    const QMetaObject *build()
    {
        static const auto s_metaObject = [](MetaObject *data) {
            constexpr auto lineCount = std::max(sizeof(ObjectType), MaximumLineCount);
            constexpr auto lines = std::make_integer_sequence<LabelId, lineCount>();
            registerMembers<ObjectType::lineOffset()>(data, lines);

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
        (registerMember<Offset + Labels>(data), ...);
    }

    template<LabelId Label>
    static void registerMember(MetaObject *data)
    {
        // the constexpr is essential here to avoid generating huge amount of code
        if constexpr (hasMember<ObjectType, Label>())
            data->emplace(ObjectType::member(detail::Tag<Label>{}));
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
template <class ObjectType, class SuperType = QObject>
class Object
    : public SuperType
{
    friend nproperty::MetaObject<ObjectType, SuperType>;
    friend nproperty::detail::MemberInfo;

public:
    using MetaObject = nproperty::MetaObject<ObjectType, SuperType>;
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

    template <auto Property>
    static consteval auto makeProperty(const char *name) noexcept
    {
        return detail::MemberInfo::makeProperty<Property>(name);
    }

    static consteval auto makeClassInfo(const char *name, const char *value) noexcept
    {
        return detail::MemberInfo::makeClassInfo(name, value);
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
