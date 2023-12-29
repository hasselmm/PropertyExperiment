#ifndef NPROPERTY_TYPETRAITS_H
#define NPROPERTY_TYPETRAITS_H

#include <algorithm>

namespace nproperty::detail {

/// Retrieve well defined addresses for objects and their members,
/// so that we can compute member offsets and the like without exploiting
/// undefined behavior like dereferencing `nullptr`.
///
class Prototype
{
public:
    template<class Object, typename Type>
    using DataMember = Type (Object::*);

    template<class Object>
    [[nodiscard]] static auto get() noexcept
    {
        return static_cast<const Object *>(buffer<sizeof(Object)>());
    }

    template<class Object, typename Type>
    [[nodiscard]] static auto get(DataMember<Object, Type> member) noexcept
    {
        return &(get<Object>()->*member);
    }

    template<class Object, typename Type>
    static consteval const Type *null(DataMember<Object, Type>) noexcept
    {
        return nullptr;
    }

private:
    static constexpr const std::size_t CommonBufferSize = 4096u;
    static consteval void checkAssertions() noexcept;

    template<std::size_t N>
    [[nodiscard]] static constexpr const void *buffer() noexcept
    {
        return s_buffer<std::max(N, CommonBufferSize)>;
    }

    template<std::size_t N>
    static constexpr char s_buffer[N] = {};
};

/// Some useful type definitions to make other definitions more expressive,
/// and compiler errors more helpful.
///
template<auto DataMember>
using ConstDataMemberType = std::remove_pointer_t<decltype(Prototype::null(DataMember))>;

template<auto DataMember>
using DataMemberType = std::remove_cv_t<ConstDataMemberType<DataMember>>;

template<class Object, typename Result, typename... Args>
using MemberFunction = Result (Object::*)(Args...);

} // namespace nproperty::detail

#endif // NPROPERTY_TYPETRAITS_H
