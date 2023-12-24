#include "ntypetraits.h"

namespace nproperty::detail {

consteval void Prototype::checkAssertions() noexcept
{
    // These assertion checks can't be free-standing as the tested members are private.
    // They also can't be in the class definition as the definition is incomplete there.
    static_assert(Prototype::buffer<1>() == Prototype::buffer<2>());
    static_assert(Prototype::buffer<1>() == Prototype::buffer<CommonBufferSize>());
}

} // namespace nproperty::detail
