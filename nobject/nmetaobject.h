#ifndef NPROPERTY_NMETAOBJECT_H
#define NPROPERTY_NMETAOBJECT_H

#include "nmetaobject_p.h"

#include <source_location>

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
            constexpr auto lines = std::make_integer_sequence<quintptr, lineCount>();
            registerMembers<ObjectType::lineOffset()>(data, lines);

            return detail::MetaObjectBuilder::build(QMetaType::fromType<ObjectType>(),
                                                    &SuperType::staticMetaObject,
                                                    &ObjectType::staticMetaObject,
                                                    &MetaObject::staticMetaCall);
        }(this);

        return s_metaObject;
    }

    template<quintptr... Indices>
    using LineNumberSequence = std::integer_sequence<quintptr, Indices...>;

    template<quintptr Offset, quintptr... Is>
    static void registerMembers(MetaObject *data, const LineNumberSequence<Is...> &)
    {
        (registerMember<Offset + Is>(data), ...);
    }

    template<quintptr Index>
    static void registerMember(MetaObject *data)
    {
        // the constexpr is essential here to avoid generating huge amount of code
        if constexpr (constexpr auto currentLine = detail::Tag<Index>{};
                      ObjectType::hasMember(currentLine))
            data->emplace(ObjectType::member(currentLine));
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
};

} // namespace nproperty

#endif // NPROPERTY_NMETAOBJECT_H
