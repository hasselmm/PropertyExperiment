#include "nobjecttest.h"

namespace nproperty {

N_OBJECT_IMPLEMENTATION(HelloWorld)
N_OBJECT_IMPLEMENTATION(NObjectMacro)
N_OBJECT_IMPLEMENTATION(NObjectModern)
N_OBJECT_IMPLEMENTATION(NObjectLegacy)

// Check if the defined properties have the expected features.

template <auto Property>
constexpr auto features = detail::DataMemberType<Property>::features();

static_assert( features<&NObjectMacro::notifying>.contains(Feature::Read));
static_assert( features<&NObjectMacro::writable> .contains(Feature::Read));

static_assert(!features<&NObjectMacro::constant> .contains(Feature::Notify));
static_assert( features<&NObjectMacro::notifying>.contains(Feature::Notify));
static_assert( features<&NObjectMacro::writable> .contains(Feature::Notify));

static_assert(!features<&NObjectMacro::constant> .contains(Feature::Write));
static_assert(!features<&NObjectMacro::notifying>.contains(Feature::Write));
static_assert( features<&NObjectMacro::writable> .contains(Feature::Write));

static_assert(!features<&NObjectMacro::constant> .contains(Feature::Reset));
static_assert(!features<&NObjectMacro::notifying>.contains(Feature::Reset));
static_assert(!features<&NObjectMacro::writable> .contains(Feature::Reset));

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

} // namespace nproperty
