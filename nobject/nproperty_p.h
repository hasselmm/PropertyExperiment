#ifndef NPROPERTY_NPROPERTY_P_H
#define NPROPERTY_NPROPERTY_P_H

#include <QtGlobal>

#include <algorithm>
#include <source_location>

namespace nproperty::detail {

/// A tagging type that's use to generate individual functions for various
/// class members. Usually the current line number is used as argument.
///
template <quintptr N>
struct Tag {};

/// A symbol name that's available at compile time,
/// and most importantly as template argument.
///
template <std::size_t N>
struct Name {
    consteval Name(const char (&str)[N], quintptr index)
        : index{index}
    {
        std::copy_n(str, N, value);
    }

    quintptr index;
    char     value[N];
};

template <std::size_t N>
consteval Name<N> name(const char (&name)[N],
                       std::source_location source = std::source_location::current())
{
    return Name<N>{name, source.line()};
}

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

} // namespace nproperty::detail

#endif // NPROPERTY_NPROPERTY_P_H
