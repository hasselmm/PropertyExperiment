#  Property Experiment

## The MObject Experiment

This was the second attempt after [AObject](../aobject/README.md).
It already allowed to fully define `QMetaObject` properties without
the help of moc, but it was ugly. So the next attempt was started with
[NObject](../nobject/README.md).

### Usage

Properties can be accessed by simple assignment, just as if they would be plain variables:

``` C++
    object.property = "new value";
    auto value = object.property;
```

Alternatively these properties also have explicit setter and getter methods:

``` C++
    object.property.set("new value");
    auto value = object.property.get();
```

Changes can be observed by connecting to a property's notifier like this:

``` C++
    connect(object, notifyMethod(&Object::property), ...);
```

If one insists on established syntax one can define setters and signals
using the `Setter` and `Signal` template:

``` C++
    connect(object, &Object::propertyChanged, ...);
    object.setProperty("new value");
    auto value = object.property();
```

### Object Definition

The header syntax is pretty good already, for what one can do without preprocessor:

``` C++
class MObjectTest : public Object<MObjectTest>
{
    M_OBJECT

public:
    using Object::Object;

    void modifyNotifying()
    {
        notifying = u"I have been changed per method"_qs;
    }

    Property<n(), QString>          constant = u"I am constant"_qs;
    Property<n(), QString, Notify> notifying = u"I am observing"_qs;
    Property<n(), QString,  Write>  writable = u"I am modifiable"_qs;

    Setter<QString> setWritable = &writable;

    static constexpr Signal<&MObjectTest::notifying> notifyingChanged = {};
    static constexpr Signal<&MObjectTest::writable>  writableChanged = {};
};
```

The `n()` function just a fancy way to generate generate a number that's unique
within this class. Any number would do. One could manually put number literals 
there. The n() function returns the from current line number. Maybe there is 
some meta programming magic to get entirely rid of it.

The `Setter` template is wasteful and inefficient, as it stores a member data
pointer to the property. That's the size of two pointers at minimum, where a
plain old member function doesn't use any instance memory at all. *Not good.*

The `Signal` syntax also is not very nice.

To finish these class definitions one has to initialize the staticMetaObject
somewhere, by putting this macro call into a source file:

``` C++
M_OBJECT_IMPLEMENTATION(MObjectTest)
```

There really is no way around, as staticMetaObject must be a static data
member. Static data members must be explicitly initialized outside the 
class definition so far.

What really kills this experiment is the way one register the properties.
Have to explicitly write such function is not nice at all:

``` C++
template<>
std::vector<MObjectTest::MetaProperty> MObjectTest::MetaObject::makeProperties()
{
    return {
        {"constant",    &null()->constant},     // FIXME: with some additional effort
        {"notifying",   &null()->notifying},    // &MObject::constant should be possible
        {"writable",    &null()->writable},
    };
}
```

---

Return to [the overview](../README.md) page.
