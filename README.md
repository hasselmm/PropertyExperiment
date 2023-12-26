# Property Experiment

This is a tiny experiment to see if C++ 20 can do QObject properties without moc.
This experiment was inspired by André Pölnitz' e-mail[^1] to the Qt Development
list: "I haven't tried yet, but I have the gut feeling that one should be able
to get away with..."

Well, yes: André was right. C++ 20 can almost replace moc these days:

``` C++
class MyObject : public Object<MyObject, QObject>
{
    M_OBJECT

public:
    using Object::Object;

    void modifyNotifying()
    {
        notifying = u"I have been changed per method"_s;
    }

    Property<n(), QString>          constant = u"I am constant"_s;
    Property<n(), QString, Notify> notifying = u"I am observing"_s;
    Property<n(), QString,  Write>  writable = u"I am modifiable"_s;

    Setter<QString> setWritable = &writable;

    static constexpr Signal<&MObject::notifying> notifyingChanged = {};
    static constexpr Signal<&MObject::writable> writableChanged = {};
};
```

The code works, and I am pretty happy with the syntax of `MyObject`.

[![Build and Test with Clang](https://github.com/hasselmm/PropertyExperiment/actions/workflows/autotest-clang.yml/badge.svg)](https://github.com/hasselmm/PropertyExperiment/actions/workflows/autotest-clang.yml)
[![Build and Test with GCC](https://github.com/hasselmm/PropertyExperiment/actions/workflows/autotest-gcc.yml/badge.svg)](https://github.com/hasselmm/PropertyExperiment/actions/workflows/cmake-autotest-gccl)
[![Build and Test with MSVC](https://github.com/hasselmm/PropertyExperiment/actions/workflows/autotest-msvc.yml/badge.svg)](https://github.com/hasselmm/PropertyExperiment/actions/workflows/autotest-msvc.yml)

Actually the `Setter` and the `Signal` fields in this class are just
syntactical sugar to preserve the API we got used to over the years.
You can fully omit them, and still get functional properties and signal:

``` C++
    // properties support simple assignment
    object.property = "new value";
    qInfo() << object.property;

    // alternatively there also are setter and getter methods
    object.property.set("new value");
    qInfo() << object.property.get();

    // connecting to connect to a signal without Signal<> field
    connect(object, notifyMethod(&Object::property), ...);
```

> [!IMPORTANT]
> Still this is just an experiment, a _proof of concept_. Many FIXMEs,
> lots of chances for improvement. Let me hear, what you think about it.
> It it worth to finish this experiment and bring it into production?

You can reach me via _mathias_ plus github at _taschenorakel_ in _de_.

## Examples

* [sobject.h](sobject.h), [sobject.cpp](sobject.cpp) —
  Standard implementation of a QObject with properties, as we are doing 
  it for decades now.
* [mobject.h](mobject.h), [mobject.cpp](mobject.cpp) -
  C++ 20 implementation of a QObject, similar to the one listed above.
* [aobject.h](aobject.h), [aobject.cpp](aobject.cpp) —
  An initial, but abdoned attempt of an C++ 20 implementation.

## Implemenatation

* [mproperty.h](mproperty.h) —
  The implementation of the C++ 20 moc replacement used by 
  [mobject.h](mobject.h).
  > This is just a proof of concept. Many shortcuts, simplifications, 
  > sharp corners and edges. Goal was to proof the concept, not to 
  > provide a production ready implementation.
* [mproperty.cpp](mproperty.cpp) —
  This file simply shall proof that [mproperty.h](mproperty.h) 
  is self-contained.
* [aproperty.h](aproperty.h), [aproperty.cpp](aproperty.cpp) —
  Obsolete, abdoned implementation, just for curiosity.

## Tests

* [main.cpp](main.cpp) —
  The main program just is a QTest fixture verifying the implementation.

## License

The code is offered under the terms of the [MIT License](LICENSE).

## Compiling, Building

You need Qt 6.5 and CMake to build. Just open `CMakeLists.txt` in
QtCreator and give it a go. Alternatively something like this:

``` Bash
mkdir build && cd build
export Qt6_DIR=/path/to/your/qt6/lib/cmake/Qt6
cmake ..
cmake --build .
```

[^1]: https://lists.qt-project.org/pipermail/development/2023-December/044807.html
