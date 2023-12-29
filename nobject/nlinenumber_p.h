#ifndef NPROPERTY_LINENUMBER_P_H
#define NPROPERTY_LINENUMBER_P_H

#include <cstdint>

#if __has_include(<source_location>)
#if (!defined(__clang_major__) || __clang_major__ > 14)
#include <source_location>
#define NPROPERTY_HAS_SOURCE_LOCATION_STANDARD
#endif
#endif

#if __has_include(<experimental/source_location>)
#include <experimental/source_location>
#define NPROPERTY_HAS_SOURCE_LOCATION_EXPERIMENTAL
#endif

#ifdef __has_builtin
#if    __has_builtin(__builtin_LINE)
#define NPROPERTY_HAS_BUILTIN_LINE
#endif
#endif

namespace nproperty::detail {
namespace linenumber {

using number = std::uint_least32_t;

template<bool builtin, bool standard, bool experimental>
struct Implementation
{
    static constexpr bool enabled() { return false; }
    static constexpr number current() { return 0; }
};

#ifdef NPROPERTY_HAS_BUILTIN_LINE

template<>
struct Implementation<true, false, false>
{
    static constexpr bool enabled() { return current() == __LINE__; }
    static constexpr number current(number line = __builtin_LINE()) { return line; }
};

#endif // NPROPERTY_HAS_BUILTIN_LINE

#ifdef NPROPERTY_HAS_SOURCE_LOCATION_STANDARD

template<>
struct Implementation<false, true, false>
{
    using location_type = std::source_location;

    static constexpr bool enabled() { return current() == __LINE__; }
    static constexpr number current(const location_type &source = location_type::current())
    {
        return source.line();
    }
};

#endif // NPROPERTY_HAS_SOURCE_LOCATION_STANDARD

#ifdef NPROPERTY_HAS_SOURCE_LOCATION_EXPERIMENTAL

template<>
struct Implementation<false, false, true>
{
    using location_type = std::experimental::source_location;

    static constexpr bool enabled() { return current() == __LINE__; }
    static constexpr number current(const location_type &source = location_type::current())
    {
        return source.line();
    }
};

#endif // NPROPERTY_HAS_SOURCE_LOCATION_EXPERIMENTAL

using Dysfunctional = Implementation<false, false, false>;
using Builtin       = Implementation<true,  false, false>;
using Standard      = Implementation<false, true,  false>;
using Experimental  = Implementation<false, false, true>;

using Selected = Implementation<Builtin::enabled(),
                                Standard::enabled() && !Builtin::enabled(),
                                Experimental::enabled() && !Builtin::enabled()>;

} // namespace linenumber

using LineNumber = linenumber::Selected;

} // namespace nproperty::detail

#endif // NPROPERTY_LINENUMBER_P_H
