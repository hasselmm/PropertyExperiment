#  Property Experiment

## The NObject Experiment

This is the next attempt after [MObject](../mobject/README.md). Goal was to remove
the hand-written member registration, and to generally reduce name duplication and
boilerplate to a minimum.

### Object Definition

For class definitions a pretty compact syntax can be used,
if one is fine with using trivial macros:

``` C++
class NObjectMacro : public nproperty::Object<NObjectMacro>
{
    N_OBJECT

public:
    using Object::Object;

    void modifyNotifying()
    {
        notifying = u"I have been changed per method"_qs;
    }

    N_PROPERTY(QString, constant)          = u"I am constant"_qs;
    N_PROPERTY(QString, notifying, Notify) = u"I am observing"_qs;
    N_PROPERTY(QString, writable,  Write)  = u"I am modifiable"_qs;
};
```

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

Changes can be observed by connecting like this:

``` C++
    object.property.connect(this, [](auto newValue) {
        ...
    });
```

For a more traditional syntax also this is working:

``` C++
    connect(&object, object.property.notifyPointer(), this, [](auto newValue) {
        ...
    });
```

If one insists on established syntax one can define setters and signals
using the `Setter` and `Signal` template:

``` C++
    connect(object, &Object::propertyChanged, ...);
    object.setProperty("new value");
    auto value = object.property();
```

To make this work the following macros can be used:

```C++
   N_PROPERTY_NOTIFICATION(someProperty)
   N_PROPERTY_SETTER(setSomeProperty, someProperty)
```

### More Traditional Object Definition

Now there are good reasons to avoid macros in modern C++.
To do so, you can define your objects like this:

``` C++
class NObjectModern : public nproperty::Object<NObjectModern>
{
    N_OBJECT

public:
    using Object::Object;

    void modifyNotifying()
    {
        notifying = u"I have been changed per method"_qs;
    }

    Property<QString, l()>          constant = u"I am constant"_qs;
    Property<QString, l(), Notify> notifying = u"I am observing"_qs;
    Property<QString, l(),  Write>  writable = u"I am modifiable"_qs;

    static constexpr nproperty::Signal<&NObjectModern::notifying> notifyingChanged = {};
    static constexpr nproperty::Signal<&NObjectModern::writable>  writableChanged  = {};

    void setWritable(QString newValue) { writable.setValue(std::move(newValue)); }

private:
    N_REGISTER_PROPERTY(constant);
    N_REGISTER_PROPERTY(notifying);
    N_REGISTER_PROPERTY(writable);
};
```

Notice that there is still this `N_REGISTER_PROPERTY()` macro. It simply hides the
following boilerplate and reduces the risk for spelling errors in the property name.

``` C++
    static consteval auto member(Tag<l()>)
    {
        return MemberInfo::make<&NObjectModern::constant>("constant");
    }
```

### Outlook:

This still needs:

* [invokable methods, generic signals](https://github.com/hasselmm/PropertyExperiment/issues/2)
* [meta object casts, interface casts](https://github.com/hasselmm/PropertyExperiment/issues/6)
* [class information](https://github.com/hasselmm/PropertyExperiment/issues/4)
* [enumerators](https://github.com/hasselmm/PropertyExperiment/issues/3) (maybe impossible)
* [custom setter and getter methods](https://github.com/hasselmm/PropertyExperiment/issues/8)

---

Return to [the overview](../README.md) page.
