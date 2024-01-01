#ifndef NPROPERTY_NCONCEPTS_H
#define NPROPERTY_NCONCEPTS_H

#include <QObject>

namespace nproperty {

/// Restrict to classes that are Qt interfaces.
/// These are abstract classes with a `Q_DECLARE_INTERFACE()` declaration.
///
template<class T>
concept QtInterface = requires { qobject_interface_iid<T *>(); }
                      && std::is_abstract_v<T>;

/// Require that the passed type is an enumeration.
///
template<class T>
concept EnumType = std::is_enum_v<T>;

/// Require that the passed type is an enumeration.
///
template<class T>
concept ScopedEnumType = std::is_enum_v<T>
                         && !std::is_convertible_v<T, std::underlying_type_t<T>>;

} // namespace nproperty

#endif // NPROPERTY_NCONCEPTS_H
