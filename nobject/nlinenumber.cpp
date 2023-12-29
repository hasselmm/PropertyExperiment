#include "nlinenumber_p.h"

namespace nproperty::detail::linenumber {

// verify properties of the dysfunctional fallback implementation

static_assert( Dysfunctional::current() == 0);
static_assert(!Dysfunctional::enabled());

// very that implementations either are functional, or otherwise are disabled

static_assert(Builtin     ::current() == __LINE__ || !Builtin     ::enabled());
static_assert(Standard    ::current() == __LINE__ || !Standard    ::enabled());
static_assert(Experimental::current() == __LINE__ || !Experimental::enabled());
static_assert(Selected    ::current() == __LINE__ || !Selected    ::enabled());

// very that Selected::current() can be used for own location dependent functions

consteval number n(number l = Selected::current()) { return l;}

static_assert(n() == __LINE__);
static_assert(n() != 0);

} // namespace nproperty::detail::linenumber
