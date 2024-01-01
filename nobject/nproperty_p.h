#ifndef NPROPERTY_NPROPERTY_P_H
#define NPROPERTY_NPROPERTY_P_H

#include <QtGlobal>

namespace nproperty::detail {

/// A tagging type that's use to generate individual functions for various
/// class members. Usually the current line number is used as argument.
///
template <quintptr N>
struct Tag {};

} // namespace nproperty::detail

#endif // NPROPERTY_NPROPERTY_P_H
