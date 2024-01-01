#include "nmetaenum.h"

namespace nproperty::metaenum {

enum class EnumClass : std::uint64_t
{
    First  = 1,
    Second = 2,
};

static_assert((std::uint64_t{1U} << 32) != 0);
static_assert((std::uint64_t{1U} << 33) != 0);

static_assert(detail::enumValue<EnumClass, 0>() == 0U);
static_assert(detail::enumValue<EnumClass, 1>() == 1U);
static_assert(detail::enumValue<EnumClass, 2>() == 2U);
static_assert(detail::enumValue<EnumClass, 3>() == 3U);

static_assert(detail::flagValue<EnumClass, 0>() == 0U);
static_assert(detail::flagValue<EnumClass, 1>() == 1U);
static_assert(detail::flagValue<EnumClass, 2>() == 2U);
static_assert(detail::flagValue<EnumClass, 3>() == 4U);
static_assert(detail::flagValue<EnumClass, 31>() == (std::uint32_t{1U} << 30));
static_assert(detail::flagValue<EnumClass, 32>() == (std::uint32_t{1U} << 31));
static_assert(detail::flagValue<EnumClass, 33>() == (std::uint64_t{1U} << 32));
static_assert(detail::flagValue<EnumClass, 64>() == (std::uint64_t{1U} << 63));

#ifdef NPROPERTY_NMETAENUM_DEBUG

std::vector<std::string_view> debug::samples()
{
    auto result = std::vector {
        name<EnumClass>(),
        detail::keyInfo<EnumClass::First>().first,
        detail::keyInfo<EnumClass::Second>().first,
    };

    for (const auto &[name, value]: metaenum::keys<EnumClass>())
        result.push_back(name);

    return result;
}

#else // NPROPERTY_NMETAENUM_DEBUG

static_assert(name<EnumClass>().empty() == false);
static_assert(name<EnumClass>() == "EnumClass");

#if !defined(Q_CC_GNU) || Q_CC_GNU >= 1200 || defined(__clang__)

static_assert(keys<EnumClass>().size() == 2);
static_assert(keys<EnumClass>()[0].first == "First");
static_assert(keys<EnumClass>()[0].second == 1);
static_assert(keys<EnumClass>()[1].first == "Second");
static_assert(keys<EnumClass>()[1].second == 2);

#endif

#endif // NPROPERTY_NMETAENUM_DEBUG

} // nproperty::metaenum
