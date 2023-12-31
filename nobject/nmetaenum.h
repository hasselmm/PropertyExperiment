#ifndef NPROPERTY_NMETAENUM_H
#define NPROPERTY_NMETAENUM_H

#include <type_traits>

namespace nproperty::detail::metaenum {

/// QFlags<T> clone that can be used as template argument:
/// In difference to QFlags<T> this type is structural.
///
template <typename Flag>
requires std::is_enum_v<Flag>
struct Flags
{
    using  FlagType = Flag;
    using ValueType = std::underlying_type_t<Flag>;

    constexpr  Flags() noexcept = default;
    constexpr  Flags(Flag flag) noexcept : Flags{static_cast<ValueType>(flag)} {}
    constexpr  Flags(ValueType value) noexcept : value{value} {}
    constexpr  Flags(const Flags &) noexcept = default;
    constexpr  Flags(Flags &&) noexcept = default;
    constexpr ~Flags() noexcept = default;

    constexpr bool operator==(const Flags &) const noexcept = default;
    constexpr bool operator==(Flag flag) const noexcept
    { return value == static_cast<ValueType>(flag); }

    constexpr Flags operator&(Flag flag) const noexcept
    { return value & static_cast<ValueType>(flag); }
    constexpr Flags operator|(Flag flag) const noexcept
    { return value | static_cast<ValueType>(flag); }

    constexpr Flags& operator|=(Flag flag) noexcept
    { value |= static_cast<ValueType>(flag); return *this; }

    constexpr operator bool() const noexcept
    { return value != 0; }

    constexpr bool contains(Flag flag) const noexcept
    { return (*this & flag) == flag; }

    ValueType value;
};

} // namespace nproperty::detail::metaenum

#endif // NPROPERTY_NMETAENUM_H
