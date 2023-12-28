#ifndef NPROPERTY_NMETAOBJECT_H
#define NPROPERTY_NMETAOBJECT_H

#include "nmetaobject_p.h"
#include "nlinenumber_p.h"

namespace nproperty {

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
    using SuperType::SuperType;

protected:
    template <typename Value, auto Name, FeatureSet Features = Feature::Read>
    using Property = nproperty::Property<ObjectType, Value, Name, Features>;

    template <std::size_t N>
    static consteval detail::Name<N> name(const char (&name)[N],
                                          quintptr label = detail::LineNumber::current())
    {
        return detail::Name{name, label};
    }

    template<typename Value, auto Name>
    void activateSignal(Value value)
    {
        auto metaCallArgs = std::array<void *, 2> { nullptr, &value };

        QMetaObject::activate(this, &ObjectType::staticMetaObject,
                              ObjectType::staticMetaObject.metaMethodForName(Name),
                              metaCallArgs.data());
    }

public: // FIXME: make signalProxy() protected again
    template<typename Value, auto Name, FeatureSet Features>
    static detail::MemberFunction<ObjectType, void, Value>
    signalProxy(const Property<Value, Name, Features> * = nullptr)
    {
        if (Q_LIKELY(canonical(Features).contains(Notify)))
            return &ObjectType::template activateSignal<Value, Name>;

        return nullptr;
    }
};

/// Alias for a properties change notification signal.
/// Use it to simulate the established Qt code style.
///
template<auto Property>
requires(detail::DataMemberType<Property>::isNotifiable())
class Signal
{
public:
    constexpr auto get() const noexcept { return detail::Prototype::get(Property)->notifyPointer(); }
    constexpr auto operator&() const noexcept { return get(); }
};

} // namespace nproperty

#endif // NPROPERTY_NMETAOBJECT_H
