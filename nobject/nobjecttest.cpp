#include "nobjecttest.h"

namespace npropertytest {

class CheckAssertions : public HelloWorld
{
    static void check()
    {
        using nproperty::detail::Tag;

        static_assert(decltype(HelloWorld::hello)::label() == 25);
        static_assert(std::is_same_v<Tag<25>, decltype(HelloWorld::hello)::TagType>);
        static_assert(std::is_same_v<Tag<25>, decltype(decltype(HelloWorld::hello)::tag())>);
        static_assert(!hasMember<24>());
        static_assert( hasMember<25>());
    }
};

N_OBJECT_IMPLEMENTATION(HelloWorld)
N_OBJECT_IMPLEMENTATION(NObjectMacro)
N_OBJECT_IMPLEMENTATION(NObjectModern)
N_OBJECT_IMPLEMENTATION(NObjectLegacy)

// Check if the defined properties have the expected features.

template <auto Property>
constexpr auto features = nproperty::detail::DataMemberType<Property>::features();

static_assert( features<&NObjectMacro::notifying>.contains(Read));
static_assert( features<&NObjectMacro::writable> .contains(Read));

static_assert(!features<&NObjectMacro::constant> .contains(Notify));
static_assert( features<&NObjectMacro::notifying>.contains(Notify));
static_assert( features<&NObjectMacro::writable> .contains(Notify));

static_assert(!features<&NObjectMacro::constant> .contains(Write));
static_assert(!features<&NObjectMacro::notifying>.contains(Write));
static_assert( features<&NObjectMacro::writable> .contains(Write));

static_assert(!features<&NObjectMacro::constant> .contains(Reset));
static_assert(!features<&NObjectMacro::notifying>.contains(Reset));
static_assert(!features<&NObjectMacro::writable> .contains(Reset));

// Check that setValue() and operator= are protected for readonly properties

template<auto (HelloWorld::*property)>
concept AssignmentPermitted = requires (HelloWorld *object) {
    (object->*property) = 0;
};

template<auto (HelloWorld::*property)>
concept SetValuePermitted = requires (HelloWorld *object) {
    (object->*property).setValue(0);
};

static_assert(!AssignmentPermitted<&HelloWorld::hello>);
static_assert( AssignmentPermitted<&HelloWorld::world>);

static_assert(!SetValuePermitted<&HelloWorld::hello>);
static_assert( SetValuePermitted<&HelloWorld::world>);

} // namespace npropertytest
