#ifndef MPROPERTY_H
#define MPROPERTY_H

#include <QObject> // FIXME: make optional?
#include <private/qmetaobjectbuilder_p.h> // FIXME: hide

#include <utility>

namespace mproperty {
namespace Private {

} // namespace Private

enum class Feature
{
    Read   = (1 << 0),
    Write  = (1 << 1),
    Reset  = (1 << 2),
    Notify = (1 << 3),
};

Q_DECLARE_FLAGS(Features, Feature)

// FIXME: maybe get this working with constexpr
// Q_DECLARE_OPERATORS_FOR_FLAGS(Features)

constexpr bool operator&(Feature a, Feature b) noexcept
{
    using ValueType = std::underlying_type_t<Feature>;
    return (static_cast<ValueType>(a) & static_cast<ValueType>(b)) != ValueType{};
}

using enum Feature;

template<class Obj, typename Ret, typename... Args>
using MemberFunction = Ret (Obj::*)(Args...);

template<class ObjectType, quintptr L, typename T, Feature F>
class Property
{
    friend ObjectType;

    Q_DISABLE_COPY_MOVE(Property)

public:
    Property() noexcept = default;
    Property(T newValue) noexcept : m_value{std::move(newValue)} {}

    static constexpr bool isReadable()   noexcept { return (F & Feature::Read) || isWritable(); } // FIXME: Feature::Write!?
    static constexpr bool isNotifiable() noexcept { return (F & Feature::Notify) || isWritable(); } // FIXME: Feature::Write!?
    static constexpr bool isWritable()   noexcept { return (F & Feature::Write); }

    // FIXME: public?
    static constexpr quintptr label() noexcept { return L; }

    T get() const noexcept { return m_value; }
    T operator()() const noexcept { return get(); }

    // FIXME public?
    MemberFunction<ObjectType, void, T> notifyPointer() const
    {
        if constexpr (isNotifiable())
            return ObjectType::signalProxy(this);

        return nullptr;
    }


// FIXME: PropertyHasRead, PropertyHasNotify, PropertyHasWrite this is all non-sense so far
// FIXME: first make ObjectType, MetaObject, ... friends, then try again
private:
    struct PropertyHasRead {};
protected:
    using PropertyHasNotify = std::conditional<isNotifiable(), T, PropertyHasRead>;
public:
    using PropertyHasWrite = std::conditional<isWritable(), T, typename PropertyHasNotify::type>;

    void set(typename PropertyHasWrite::type newValue)
    {
        if constexpr (isNotifiable()) {
            if (std::exchange(m_value, std::move(newValue)) != m_value)
                notify(m_value);
        } else {
            m_value = std::move(newValue);
        }
    }

    Property &operator=(typename PropertyHasWrite::type newValue)
    {
        set(std::move(newValue));
        return *this;
    }

public: // FIXME: maybe mark these accessors protected or private
    quintptr address() const noexcept
    {
        return reinterpret_cast<quintptr>(this);
    }

    quintptr offset() const noexcept
    {
        for (const auto &p: ObjectType::MetaObject::properties())
            if (p.label() == label())
                return p.offset();

        return 0u;
    }

    ObjectType *object() const noexcept
    {
        for (const auto &p: ObjectType::MetaObject::properties()) {
            if (p.label() == label())
                return reinterpret_cast<ObjectType *>(address() - p.offset());
        }

        return nullptr;
    }

protected:
    void notify(typename PropertyHasNotify::type newValue)
    {
        const auto target = object();
        Q_ASSERT(target != nullptr);

        (target->*ObjectType::signalProxy(this))(std::move(newValue));
    }

private:
    T m_value;
};

struct MetaMethod
{
    constexpr MetaMethod() noexcept = default;
    constexpr MetaMethod(const void *pointer, quintptr label) noexcept
        : pointer{pointer}, label{label} {}

    const void *pointer;
    quintptr    label;
};

template<class ObjectType>
class MetaObject;

// FIXME: simplify, attribute visibility, friends
template<class ObjectType>
class MetaProperty
{
public:
    template<quintptr L, typename T, Feature F>
    using Property = mproperty::Property<ObjectType, L, T, F>;
    using MetaObject = mproperty::MetaObject<ObjectType>;

    template<quintptr L, typename T, Feature F>
    MetaProperty(const char *name, const Property<L, T, F> *p)
        : m_name{name}
        , m_type{QMetaType::fromType<T>()}
        , m_read{makeReadFunction(p)}
        , m_write{makeWriteFunction(p)}
        , m_notify{makeNotifyPointer(p)}
        , m_offset{MetaObject::offsetOf(p)}
        , m_label{p->label()}
        , m_features{F}
    {}

    auto name()          const noexcept { return m_name; }
    auto type()          const noexcept { return m_type; }
    auto notifyPointer() const noexcept { return m_notify; }
    auto offset()        const noexcept { return m_offset; }
    auto label()         const noexcept { return m_label; }

    bool isReadable()    const noexcept { return (m_features & Feature::Read) || isNotifyable(); } // FIXME
    bool isNotifyable()  const noexcept { return (m_features & Feature::Notify) || isWritable(); } // FIXME
    bool isWritable()    const noexcept { return m_features & Feature::Write; }
    bool isResettable()  const noexcept { return m_features & Feature::Reset; }

    void read(const ObjectType *object, void *value) const { m_read(object, value); }
    void write(ObjectType *object, const void *value) const { m_write(object, value); }

private:
    using ReadFunction = void (*)(const ObjectType *, void *);
    using WriteFunction = void (*)(ObjectType *, const void *);

    template<quintptr L, typename T, Feature F>
    static ReadFunction makeReadFunction(const Property<L, T, F> *prototype)
    {
        static const auto offset = MetaObject::offsetOf(prototype);
        Q_ASSERT(static_cast<std::size_t>(offset) < sizeof(ObjectType));

        return [](const ObjectType *object, void *value) {
            const auto p = MetaObject::template property<L, T, F>(object, offset);
            *reinterpret_cast<T *>(value) = p->get();
        };
    }

    template<quintptr L, typename T, Feature F>
    static WriteFunction makeWriteFunction(const Property<L, T, F> *prototype)
    {
        if constexpr (F & Feature::Write) {
            static const auto offset = MetaObject::offsetOf(prototype); // FIXME: this works only once
            Q_ASSERT(static_cast<std::size_t>(offset) < sizeof(ObjectType));

            return [](ObjectType *object, const void *value) {
                const auto p = MetaObject::template property<L, T, F>(object, offset);
                p->set(*reinterpret_cast<const T *>(value));
            };
        } else {
            return {};
        }
    }

    template<quintptr L, typename T, Feature F>
    static const void *makeNotifyPointer(const Property<L, T, F> *prototype)
    {
        union {
            MemberFunction<ObjectType, void, T> f;
            const void *p;
        } aliasing;

        aliasing.f = prototype->notifyPointer();
        return aliasing.p;
    }

    const char      *m_name;
    QMetaType        m_type;
    ReadFunction     m_read;
    WriteFunction    m_write;
    const void      *m_notify;
    quintptr         m_offset;
    quintptr         m_label;
    Features         m_features;
};

// FIXME: simplify, attribute visibility, friends
template<class ObjectType>
class MetaObject
    : public QMetaObject
{
public:
    template<quintptr L, typename T, Feature F>
    using Property = mproperty::Property<ObjectType, L, T, F>;
    using MetaProperty = mproperty::MetaProperty<ObjectType>;
    using QMetaObject::property;

    MetaObject() : QMetaObject{make()} {}

    static constexpr auto null() noexcept { return static_cast<const ObjectType *>(nullptr); }

    template<quintptr L, typename T, Feature F>
    static quintptr offsetOf(const Property<L, T, F> *prototype)
    {
        return reinterpret_cast<quintptr>(prototype)
               - reinterpret_cast<quintptr>(null());
    }

    template<quintptr L, typename T, Feature F>
    static const Property<L, T, F> *property(const ObjectType *object, quintptr offset)
    {
        const auto address = reinterpret_cast<const char *>(object) + offset;
        return reinterpret_cast<const Property<L, T, F> *>(address);
    }

    template<quintptr L, typename T, Feature F>
    static Property<L, T, F> *property(ObjectType *object, quintptr offset)
    {
        const auto address = reinterpret_cast<char *>(object) + offset;
        return reinterpret_cast<Property<L, T, F> *>(address);
    }

    QMetaObject make()
    {
        auto objectBuilder = QMetaObjectBuilder{};

        objectBuilder.setClassName(QMetaType::fromType<ObjectType>().name());
        objectBuilder.setSuperClass(&ObjectType::BaseType::staticMetaObject);
        objectBuilder.setFlags(PropertyAccessInStaticMetaCall);

        for (const MetaProperty &p: properties()) {
            auto propertyBuilder = objectBuilder.addProperty(p.name(), p.type().name(), p.type());

            propertyBuilder.setReadable(p.isReadable());
            propertyBuilder.setWritable(p.isWritable());
            propertyBuilder.setResettable(p.isResettable());
            propertyBuilder.setDesignable(true);
            propertyBuilder.setScriptable(true);
            propertyBuilder.setStored(true);
            propertyBuilder.setStdCppSet(p.isWritable()); // QTBUG-120378
            propertyBuilder.setFinal(true);

            if (p.notifyPointer()) {
                auto signature = propertyBuilder.name() + "Changed(" + p.type().name() + ")";
                auto notifyBuilder = objectBuilder.addSignal(std::move(signature));
                notifyBuilder.setParameterNames({propertyBuilder.name()});
                propertyBuilder.setNotifySignal(std::move(notifyBuilder));
            } else {
                propertyBuilder.setConstant(true);
            }
        }

        objectBuilder.setStaticMetacallFunction([](QObject *object, QMetaObject::Call call,
                                                   int offset, void **args) {
            const auto index = static_cast<std::size_t>(offset);

            if (call == QMetaObject::ReadProperty) {
                if (index < properties().size()) {
                    const auto &p = properties()[index];
                    p.read(static_cast<const ObjectType *>(object), args[0]);
                }
            } else if (call == QMetaObject::WriteProperty) {
                if (index < properties().size()) {
                    const auto &p = properties()[index];
                    p.write(static_cast<ObjectType *>(object), args[0]);
                }
            } else if (call == QMetaObject::IndexOfMethod) {
                const auto result = reinterpret_cast<int *>(args[0]);
                const auto search = *reinterpret_cast<void **>(args[1]);

                auto methodIndex = 0;

                for (auto i = 0u; i < properties().size(); ++i) {
                    const auto notify = properties()[i].notifyPointer();

                    if (notify == search) {
                        *result = methodIndex;
                        break;
                    } else if (notify != nullptr) {
                        ++methodIndex;
                    }
                }
            } else {
                qWarning("Unsupported metacall for %s: call=%d, offset=%d, args=%p",
                         ObjectType::staticMetaObject.className(), call, offset,
                         static_cast<const void *>(args));
            }
        });

        return *objectBuilder.toMetaObject(); // FIXME: memory leak
    }

// FIXME private:
    static std::vector<MetaMethod> makeMethods()
    {
        auto methods = std::vector<MetaMethod>{};

        for (const MetaProperty &p: properties()) {
            if (const auto notify = p.notifyPointer())
                methods.emplace_back(notify, p.label());
        }

        return methods;
    }

    static const std::vector<MetaMethod> &methods()
    {
        const static auto s_methods = makeMethods();
        return s_methods;
    }

    static std::vector<MetaProperty> makeProperties();
    static const std::vector<MetaProperty> &properties()
    {
        const static auto s_properties = makeProperties();
        return s_properties;
    }
};

template<class ObjectType, typename T>
class Setter
{
public:
    template<quintptr L, Feature F>
    Setter(Property<ObjectType, L, T, F> *property)
        : m_setter{makeFunction(property)}
    {}

    void operator()(T newValue) const // FIXME: remove const?
    {
        m_setter(std::move(newValue));
    }

private:
    using SetterFunction = std::function<void (T)>;

    template<quintptr L, Feature F>
    static SetterFunction makeFunction(Property<ObjectType, L, T, F> *property)
    {
        return [property](T &&newValue) {
            property->set(std::forward<T>(newValue));
        };
    }

    SetterFunction m_setter;
};

// FIXME: move, make constexpr
template<class ObjectType, quintptr L, typename T, Feature F>
static auto notifyMethod(Property<ObjectType, L, T, F> (ObjectType::*property))
{
    return (ObjectType::MetaObject::null()->*property).notifyPointer();
}

template<auto property>
class Signal
{
public:
    constexpr auto get() const noexcept { return notifyMethod(property); }
    constexpr auto operator&() const noexcept { return get(); }
};

///
/// A base type for QObject based classes
///
template<class ObjectType, class BaseObjectType = QObject>
requires std::is_base_of_v<QObject, BaseObjectType>
class Object
    : public BaseObjectType
{
public:
    using BaseType = BaseObjectType;
    using MetaObject = mproperty::MetaObject<ObjectType>;
    using MetaProperty = mproperty::MetaProperty<ObjectType>;

    using BaseType::BaseType;

    const QMetaObject *metaObject() const override { return &ObjectType::staticMetaObject; } // FIXME

protected:
    template<quintptr L, typename T, Feature F = Feature::Read>
    using Property = mproperty::Property<ObjectType, L, T, F>;

    template<typename T>
    using Setter = mproperty::Setter<ObjectType, T>;

#if defined(Q_CC_CLANG) && Q_CC_CLANG < 1600

    static constexpr quintptr n(quintptr l = __builtin_LINE()) noexcept
    {
        return l;
    }

#else // defined(Q_CC_CLANG) && Q_CC_CLANG < 1600

    static constexpr quintptr n(const std::source_location &location =
                               std::source_location::current()) noexcept
    {
        return location.line();
    }

#endif // defined(Q_CC_CLANG) && Q_CC_CLANG < 1600

public:
// FIXME: protected or even private
    template<quintptr L, typename T, Feature F>
    static MemberFunction<ObjectType, void, T> signalProxy(const Property<L, T, F> *)
    {
        return &ObjectType::template activateSignal<L, T>;
    }

    template<quintptr L, typename T>
    void activateSignal(T value)
    {
        // FIXME: maybe create hash
        for (auto i = 0U; i < MetaObject::methods().size(); ++i) {
            if (MetaObject::methods().at(i).label == L) {
                void *args[] = {
                    nullptr,
                    const_cast<void*>(reinterpret_cast<const void*>(std::addressof(value)))
                };

                QMetaObject::activate(this, &ObjectType::staticMetaObject,
                                      static_cast<int>(i), args);
            }
        }
    }
};

} // namespace mproperty

#define M_OBJECT                                                                \
                                                                                \
public:                                                                         \
    static const MetaObject staticMetaObject;                                   \
    int qt_metacall(QMetaObject::Call call, int offset, void **args) override;  \
    static_assert(n() == __LINE__);

#define M_OBJECT_IMPLEMENTATION(ClassName, ...)                                 \
                                                                                \
int ClassName::qt_metacall(QMetaObject::Call call, int offset, void **args)     \
{                                                                               \
    return staticMetaObject.static_metacall(call, offset, args);                \
}                                                                               \
                                                                                \
const ClassName::MetaObject ClassName::staticMetaObject = {};

#endif // MPROPERTY_H
