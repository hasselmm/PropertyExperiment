#ifndef APROPERTY_APROPERTY_H
#define APROPERTY_APROPERTY_H

#include <utility>

namespace aproperty {
namespace Private {

template<class T, typename V>
using NotifyMemberFunction = void (T::*)(V);

template<class T, typename V>
struct NotifyMemberTrait
{
    using TargetType = T;
    using ValueType = V;
};

template<class T, typename V>
constexpr auto inspect(NotifyMemberFunction<T, V>)
{
    return NotifyMemberTrait<T, V>{};
}

template<auto notify>
struct NotifierInfo
{
    using NotifierType = decltype(inspect(notify));
    using TargetType = typename NotifierType::TargetType;
    using ValueType = typename NotifierType::ValueType;
};

} // namespace

template<typename T>
class Property
{
public:
    using ValueType = T;

    Property() noexcept = default;
    Property(ValueType initialValue) noexcept
        : m_value{std::move(initialValue)}
    {}

    constexpr auto get() const noexcept { return m_value; }
    auto operator()() const noexcept { return get(); }

protected:
    ValueType m_value;
};

template<typename> class Setter;

template<auto notify>
class Notifying : public Property<typename Private::NotifierInfo<notify>::ValueType>
{
public:
    using TargetType = typename Private::NotifierInfo<notify>::TargetType;
    using ValueType = typename Private::NotifierInfo<notify>::ValueType;

    Notifying(TargetType *target, ValueType initialValue) noexcept
        : Property<ValueType>{std::move(initialValue)}
        , m_target{target}
    {}

protected:
    void set(ValueType newValue)
    {
        if (std::exchange(m_value, std::move(newValue)) != m_value)
            (m_target->*notify)(m_value);
    }

    Notifying &operator=(ValueType newValue)
    {
        set(std::move(newValue));
        return *this;
    }

private:
    TargetType *const m_target;
    using Property<ValueType>::m_value;

    friend Setter<ValueType>;
    friend TargetType;
};

template<typename T>
class Setter
{
public:
    using ValueType = T;

    template<auto notify>
    Setter(Notifying<notify> *property)
        : m_property{property}
        , m_setValue{[](void *p, ValueType newValue) {
            const auto property = static_cast<Notifying<notify> *>(p);
            property->set(std::move(newValue));
        }}
    {}

    void operator()(ValueType newValue) const
    {
        m_setValue(m_property, std::move(newValue));
    }

private:
    // Using std::function<void(T)> would give Setter<T> a size of 32 bytes on x86_64.
    // By manually doing the type-erasure the size of Setter<T> is reduced to 16 bytes.

    void *m_property;
    void (* m_setValue)(void *, ValueType);
};

} // namespace aproperty

#endif // APROPERTY_APROPERTY_H
