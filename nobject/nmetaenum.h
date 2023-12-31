#ifndef NPROPERTY_NMETAENUM_H
#define NPROPERTY_NMETAENUM_H

#include "nconcepts.h"

#undef NPROPERTY_NMETAENUM_DEBUG

/// This tiny header-only library implements C++ reflection for enum-types. It uses
/// the trick introduced by [Magic Enum C++](https://github.com/Neargye/magic_enum/):
/// The `__PRETTY_FUNCTION__` macro of modern compilers and friends provides template
/// information. This can be parsed and used for reflection. Most recent compilers
/// also provide this information in the standard compliant `std::source_location`
/// class. Maybe such more standards compiliant implementation should be added,
/// eventhough the parsing still would be compiler specific.
///
namespace nproperty::metaenum {

using KeyInfo      = std::pair<std::string_view, int>;
using KeyInfoArray = std::vector<KeyInfo>;

namespace detail {

/// Disable warnings about out-of-domain conversions enum types.
/// Some compilers just have become "too" smart these days.
///
#if defined(Q_CC_CLANG) && Q_CC_CLANG >= 1600
#define NPROPERTY_WARNING_DISABLE_ENUM_CONSTEXPR_CONVERSION() \
QT_WARNING_DISABLE_CLANG("-Wenum-constexpr-conversion")
#else
#define NPROPERTY_WARNING_DISABLE_ENUM_CONSTEXPR_CONVERSION() \
QT_WARNING_DISABLE_GCC("-Wconversion")
#endif

/// Extract first template argument from a function signature. It either can be found
/// between function name and argument list within angle brackets (MSVC), or it's behind
/// the argument list in square brackets (Clang and GCC). Template arguments are separated
/// by comma (MSVC), or semicolon (Clang and GCC).
///
[[nodiscard]] constexpr std::string_view firstTemplateArgument(std::string_view signature) noexcept
{
#if defined(Q_CC_MSVC) && !defined(__clang__)
    // MSVC puts the template information about std::string_view in front. It contains
    // angle brackets, which MSVC also uses to annotate the current function. Therefore
    // we must first find the function name. The namespace is the same for each function
    // where. Starting with the name instead of colons avoids backtracking from mismatches.
    const auto namep = signature.find("metaenum::");
    const auto first = signature.find('<', namep);
    const auto last  = signature.find_first_of(">,", first);
#else
    const auto first = signature.find('[');
    const auto last  = signature.find_first_of("];", first);
#endif

    if (!std::is_constant_evaluated()) {
        Q_ASSERT(first != std::string_view::npos);
        Q_ASSERT(last  != std::string_view::npos);
    }

    return signature.substr(first + 1, last - first - 1);
}

/// Find name and value of an enumeration member.
///
/// NOTE: Giving the template argument T a name is essential,
/// as Clang hides anonymous template arguments in its __PRETTY_FUNCTION__
///
template<auto value, EnumType = decltype(value)>
[[nodiscard]] constexpr KeyInfo keyInfo()
{
    // GCC 13.1
    //
    //  "nproperty::detail::KeyInfo nproperty::detail::metaenum::keyInfo() "
    //  "[with auto value = npropertytest::NObjectMacro::NoError; "
    //  "<template-parameter-1-2> = npropertytest::NObjectMacro::Error; "
    //  "nproperty::detail::KeyInfo = std::pair<std::basic_string_view<char>, int>]
    //
    auto name = firstTemplateArgument(Q_FUNC_INFO);

    // first last colon
    if (const auto pos = name.find_last_of(":)");
        pos != std::string::npos && pos > 0)
        name = name.substr(pos + 1);

    QT_WARNING_PUSH
    NPROPERTY_WARNING_DISABLE_ENUM_CONSTEXPR_CONVERSION()

    return {name, static_cast<int>(value)};

    QT_WARNING_POP
}

/// Compute a possible enum value for the given `Index`.
///
template<EnumType T, std::size_t Index>
[[nodiscard]] constexpr std::underlying_type_t<T> enumValue() noexcept
{
    using U = std::underlying_type_t<T>;
    return static_cast<U>(Index);
}

/// Compute a possible flag value for the given `Index`.
///
template<EnumType T, std::size_t Index>
[[nodiscard]] constexpr std::underlying_type_t<T> flagValue() noexcept
{
    using U = std::underlying_type_t<T>;

    if constexpr (Index > 0)
        return (static_cast<U>(1) << static_cast<U>(Index - 1));

    return static_cast<U>(0);
}

/// Convert an index-sequence into a KeyValue-sequence.
///
template<EnumType T, bool isFlag, std::size_t... Is>
[[nodiscard]] constexpr std::array<KeyInfo, sizeof...(Is)>
keyValueSequence(const std::index_sequence<Is...> &)
{
    QT_WARNING_PUSH
    NPROPERTY_WARNING_DISABLE_ENUM_CONSTEXPR_CONVERSION()

    if constexpr (isFlag) {
        return { keyInfo<static_cast<T>(flagValue<T, Is>())>()..., };
    } else {
        return { keyInfo<static_cast<T>(enumValue<T, Is>())>()..., };
    }

    QT_WARNING_POP
}

template<EnumType T, bool isFlag, std::size_t N>
[[nodiscard]] constexpr auto makeKeyScanSequence()
{
    if constexpr (isFlag) {
        return std::make_index_sequence<sizeof(T) * 8U>{};
    } else {
        return std::make_index_sequence<(N > 0U) ? N : 256>{};
    }
}

} // namespace detail

/// Check if a given `KeyInfo` is valid.
///
[[nodiscard]] constexpr bool isValid(const KeyInfo &key) noexcept
{
    const auto &[name, value] = key;

    if (name.empty())
        return false;
    if (name[0] >= '0' && name[0] <= '9')
        return false; // digit
    if (name[0] == '-')
        return false;

    return true;
};

/// Find key values for the enum-type `T`. This works by scaning value
/// candidates at compile-time. For regular enum-types each value in the
/// range [0..`N`-1] is tested. For flag-types only zero and powers of two
/// are tested.
///
template<EnumType T, bool isFlag = false, std::size_t N = 256>
[[nodiscard]] constexpr KeyInfoArray keys() noexcept
{
    constexpr auto keyScanSequence = detail::makeKeyScanSequence<T, isFlag, N>();
    constexpr auto keySequence = detail::keyValueSequence<T, isFlag>(keyScanSequence);
    constexpr auto keyCount = std::ranges::count_if(keySequence, isValid);

#ifndef NPROPERTY_NMETAENUM_DEBUG
    static_assert(keyCount > 0);
#endif

    auto result = KeyInfoArray{};
    result.reserve(keyCount);

    std::ranges::copy_if(keySequence, std::back_inserter(result), isValid);

    return result;
}

/// Find an enum-type's name.
///
/// NOTE: Giving the template argument T a name is essential,
/// as Clang hides anonymous template arguments in its __PRETTY_FUNCTION__
///
template<EnumType T>
[[nodiscard]] constexpr std::string_view name() noexcept
{
    // GCC 13.1
    //
    //  "constexpr std::string_view nproperty::detail::metaenum::name() "
    //  "[with T = npropertytest::NObjectMacro::Error; "
    //  "std::string_view = std::basic_string_view<char>]
    //
    // Clang 17
    //
    //  "std::string_view nproperty::detail::metaenum::name() "
    //  "[T = apropertytest::NObjectMacro::Error]"
    //
    // MSVC 2019
    //
    //  "class std::basic_string_view<char,struct std::char_traits<char> > "
    //  "__cdecl nproperty::detail::metaenum::name<enum apropertytest::"
    //  "MObjectMacro::Error>() noexcept"
    //

    auto name = detail::firstTemplateArgument(Q_FUNC_INFO);

    // "with T ="
    if (const auto pos = name.find_last_of("=");
        pos != std::string::npos && pos > 0)
        name = name.substr(pos + 1);

    // skip whitespace after "="
    if (const auto pos = name.find_first_not_of(" ");
        pos != std::string::npos && pos > 0)
        name = name.substr(pos);

    // Drop namespaces: GCC 12 and earlier strip common parts with the function,
    // which makes it pretty impossible to parse them. One could work around by
    // using exotic and special namespaces for the reflection functions, but we
    // don't need need the namespace part right now. Therefore let's worry later,
    // if really needed.
    if (const auto pos = name.rfind(':');
        pos != std::string::npos && pos > 0)
        name = name.substr(pos + 1);

    // MSVC (and maybe others) put an "enum" in front of the typename
    if (constexpr auto prefix = std::string_view{"enum "};
        name.starts_with(prefix))
        name = name.substr(prefix.size());

    return name;
}

/// QFlags<T> clone that can be used as template argument:
/// In difference to QFlags<T> this type is structural.
///
template<EnumType Flag>
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

#ifdef  NPROPERTY_NMETAENUM_DEBUG

namespace debug {
std::vector<std::string_view> samples();
}

#endif // NPROPERTY_NMETAENUM_DEBUG

} // namespace nproperty::metaenum

#endif // NPROPERTY_NMETAENUM_H
