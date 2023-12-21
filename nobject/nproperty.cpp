#include "nproperty.h"

namespace nproperty {

static_assert(canonical(Feature::Read)   == (Feature::Read));
static_assert(canonical(Feature::Notify) == (Feature::Read | Feature::Notify));
static_assert(canonical(Feature::Reset)  == (Feature::Read | Feature::Notify | Feature::Reset));
static_assert(canonical(Feature::Write)  == (Feature::Read | Feature::Notify | Feature::Write));

} // namespace nproperty
