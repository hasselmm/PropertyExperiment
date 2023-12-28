# Property Experiment

## Motivation

This is a tiny experiment to see if C++ 20 can do QObject properties without moc.
This experiment was inspired by André Pölnitz' e-mail[^1] to the Qt Development
list: "I haven't tried yet, but I have the gut feeling that one should be able
to get away with..."

Well, yes: André was right. C++ 20 can almost replace moc these days:

``` C++
class NObjectTest : public nproperty::Object<NObjectTest>
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

    N_PROPERTY_NOTIFICATION(notifying)
    N_PROPERTY_NOTIFICATION(writable)

    N_PROPERTY_SETTER(setWritable, writable)
};
```

The code works, and I am pretty happy with the syntax of `NObject`.

[![Build and Test with Clang](https://github.com/hasselmm/PropertyExperiment/actions/workflows/autotest-clang.yml/badge.svg)](https://github.com/hasselmm/PropertyExperiment/actions/workflows/autotest-clang.yml)
[![Build and Test with GCC](https://github.com/hasselmm/PropertyExperiment/actions/workflows/autotest-gcc.yml/badge.svg)](https://github.com/hasselmm/PropertyExperiment/actions/workflows/cmake-autotest-gccl)
[![Build and Test with MSVC](https://github.com/hasselmm/PropertyExperiment/actions/workflows/autotest-msvc.yml/badge.svg)](https://github.com/hasselmm/PropertyExperiment/actions/workflows/autotest-msvc.yml)

> [!IMPORTANT]
> Still this is just an experiment, a _proof of concept_. Many FIXMEs,
> lots of chances for improvement. Let me hear, what you think about it.
> It it worth to finish this experiment and bring it into production?

You can reach me via _mathias_ plus github at _taschenorakel_ in _de_.

## The Various Experiments

More detailed documentation of the several experiments
can be found in their subfolders:

* The initial, but abdoned [AObject Experiment](aobject/README.md)
  can be found in the [aobject folder](aobject).
* A much better attempt was the [MObject Experiment](mobject/README.md).
  It already allowed to fully define `QMetaObject` properties without
  the help of moc, but it was ugly. It can be found in the
  [mobject folder](mobject).
* The [NObject Experiment](nobject/README.md) in the [nobject folder](nobject)
  is my current favorite. Its usage is pretty similiar to the example above.
  By the number of files in this folder one could argue the experiment got
  out of control by now. Still I am planing to add the missing `QMetaObject`
  features beyond simple properties. Actually it seems like mechanisms behind
  work well enough that I could try to create a plain C++ version without
  any help from Qt. That would mean full `QMetaObject` reflection with plain
  C++. Wouldn't that be pretty cool?
* The [SObject Experiment](sobject/README.md) in the [sobject folder](sobject)
  is just the plain old standard mechanism we are using with Qt for decades now.

## Main Program

* The [main program](main.cpp) just is a QTest fixture to verifying
  the implementations. It contains a few pretty nice template tricks.

## Compiling, Building

You need Qt 6.5 and CMake to build. Just open `CMakeLists.txt` in
QtCreator and give it a go. Alternatively something like this:

``` Bash
mkdir build && cd build
export Qt6_DIR=/path/to/your/qt6/lib/cmake/Qt6
cmake ..
cmake --build .
```

## License

The code is offered under the terms of the [MIT License](LICENSE).

---

[^1]: https://lists.qt-project.org/pipermail/development/2023-December/044807.html
