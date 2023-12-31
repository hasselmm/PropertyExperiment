#include "backports.h"

#include "aobject/aobjecttest.h"
#include "mobject/mobjecttest.h"
#include "nobject/nobjecttest.h"
#include "sobject/sobjecttest.h"

#include <QSignalSpy>
#include <QTest>

namespace {

using apropertytest::AObjectTest;
using mpropertytest::MObjectTest;
using npropertytest::NObjectMacro;
using npropertytest::NObjectModern;
using npropertytest::NObjectLegacy;
using spropertytest::SObjectTest;

/// The following is a system of concepts, constants and flags that
/// allow to disable test features which cannot get compiled, or simply
/// are not implemented yet for some object type.
///
enum Feature
{
    None                    = 0,
    MetaObject              = 1 << 0,
    PropertyDefinitions     = 1 << 1,
    UniquePropertyIds       = 1 << 2,
    PropertyAddresses       = 1 << 3,
    MethodDefinitions       = 1 << 4,
    SignalAddresses         = 1 << 5,
    PropertyChanges         = 1 << 6,
    PropertyNotifications   = 1 << 7,
    NotifyPointers          = 1 << 8,
    ClassInfo               = 1 << 9,
    Interfaces              = 1 << 10,
    Enumerators             = 1 << 11,
};

/// By default all features are considered enabled, and no features are skipped.
///
template<class T = void> constexpr auto implementedFeatures = ~Feature::None;
template<class T = void> constexpr auto skippedFeatures = Feature::None;

/// Now the constants and concepts to disable features.
///
template<Feature feature, class T>
constexpr bool isImplementedFeature = (implementedFeatures<T> & feature) == feature;

template<Feature feature, class T>
constexpr bool isSkippedFeature = (skippedFeatures<T> & feature) == feature;

template<class T, Feature feature>
concept HasFeature = (isImplementedFeature<feature, T>
                      && !isSkippedFeature<feature, T>);

/// Finally the type specific feature configurations.
///
template<> constexpr auto implementedFeatures<AObjectTest>
    = implementedFeatures<>
      & ~UniquePropertyIds      // properties do not have their own objects with moc
      & ~PropertyAddresses      // properties do not have their own objects with moc
      & ~NotifyPointers         // properties do not have their own objects with moc
    ;

template<> constexpr auto implementedFeatures<SObjectTest>
    = implementedFeatures<>
      & ~UniquePropertyIds      // properties do not have their own objects with moc
      & ~PropertyAddresses      // properties do not have their own objects with moc
      & ~NotifyPointers         // properties do not have their own objects with moc
    ;

template<> constexpr auto implementedFeatures<MObjectTest>
    = implementedFeatures<>
      & ~ClassInfo
      & ~Interfaces
      & ~Enumerators
    ;

template<> constexpr auto skippedFeatures<NObjectLegacy>
    = skippedFeatures<>
      | SignalAddresses
      | PropertyChanges
      | PropertyNotifications
      | NotifyPointers
    ;

/// Just a tiny wrapper with simple name for the pretty verbose
/// `!std::is_member_function_pointer_v<decltype(&T::member)>`.
///
template<auto pointer>
constexpr bool isDataMember = std::is_member_pointer_v<decltype(pointer)>
                              && !std::is_member_function_pointer_v<decltype(pointer)>;

/// Allow to run templated test function for different types
///
#define MAKE_TESTDATA(Feature) \
    makeTestData<Feature, decltype([](auto &object) { \
        test##Feature(object); \
    })>()


/// Simply show an expression and its value.
///
#define SHOW(What) qInfo() << #What " =>" << (What)


/// Some constants for property related tests
///
const auto  constant1 = u"I am constant"_qs;
const auto notifying1 = u"I am observing"_qs;
const auto notifying2 = u"I have been changed per method"_qs;
const auto  writable1 = u"I am modifiable"_qs;
const auto  writable2 = u"I have been changed per setter"_qs;
const auto  writable3 = u"I have been changed by assignment"_qs;
const auto  metacall1 = u"I have been changed per metacall"_qs;

const auto notifyingSpy1 = QList<QVariantList>{};
const auto notifyingSpy2 = QList<QVariantList>{{notifying2}};

const auto writableSpy1  = QList<QVariantList>{};
const auto writableSpy2  = QList<QVariantList>{{writable2}};
const auto writableSpy3  = QList<QVariantList>{{writable2}, {metacall1}};
const auto writableSpy4  = QList<QVariantList>{{writable2}, {metacall1}, {writable3}};

/// The property experiment is implemented as Qt Test suite.
///
class PropertyExperiment: public QObject
{
    Q_OBJECT

    /// --------------------------------------------------------------------------------------------
    /// Normally one would just make the type under test template argument of this class.
    /// Unfortunately moc doesn't support template types. Therefore a different approach
    /// is needed here to provide (almost) identical tests for each property system.
    ///
    /// The actual tests are implemented in template functions. The _data() slots of each
    /// test simply push a type erased delegate to the test data set, which then is run
    /// by the actual test slot. Sounds complicated, but isn't too much. Foremost it's
    /// pretty canonical as you can see below.
    /// --------------------------------------------------------------------------------------------

private slots:
    void testMetaObject()                   { runFeatureTest(); }
    void testMetaObject_data()              { MAKE_TESTDATA(MetaObject); }

    void testPropertyDefinitions()          { runFeatureTest(); }
    void testPropertyDefinitions_data()     { MAKE_TESTDATA(PropertyDefinitions); }

    void testUniquePropertyIds()            { runFeatureTest(); }
    void testUniquePropertyIds_data()       { MAKE_TESTDATA(UniquePropertyIds); }

    void testPropertyAddresses()            { runFeatureTest(); }
    void testPropertyAddresses_data()       { MAKE_TESTDATA(PropertyAddresses); }

    void testMethodDefinitions()            { runFeatureTest(); }
    void testMethodDefinitions_data()       { MAKE_TESTDATA(MethodDefinitions); }

    void testSignalAddresses()              { runFeatureTest(); }
    void testSignalAddresses_data()         { MAKE_TESTDATA(SignalAddresses); }

    void testNotifyPointers()               { runFeatureTest(); }
    void testNotifyPointers_data()          { MAKE_TESTDATA(NotifyPointers); }

    void testPropertyChanges()              { runBenchmark(); }
    void testPropertyChanges_data()         { MAKE_TESTDATA(PropertyChanges); }

    void testPropertyNotifications()        { runBenchmark(); }
    void testPropertyNotifications_data()   { MAKE_TESTDATA(PropertyNotifications); }

    void testClassInfo()                    { runFeatureTest(); }
    void testClassInfo_data()               { MAKE_TESTDATA(ClassInfo); }

    void testInterfaces()                   { runBenchmark(); }
    void testInterfaces_data()              { MAKE_TESTDATA(Interfaces); }

    void testEnumerators()                  { runFeatureTest(); }
    void testEnumerators_data()             { MAKE_TESTDATA(Enumerators); }

    /// --------------------------------------------------------------------------------------------
    /// An NObject specific test, testing its HelloWorld class and unique features
    /// --------------------------------------------------------------------------------------------

    void testNObject()
    {
        using npropertytest::HelloWorld;

        auto object     = HelloWorld{};
        auto metaObject = object.staticMetaObject;

        SHOW(object.hello.label());
        SHOW(object.world.label());

        SHOW(object.hello.name());
        SHOW(object.world.name());

        SHOW(object.hello.value());
        SHOW(object.world.value());

        SHOW(object.hello());
        SHOW(object.world());

        SHOW(object.hello);
        SHOW(object.world);

        const auto indices = QSet{object.hello.label(),
                                  object.world.label()};

        QCOMPARE(indices.count(), 2);

        QCOMPARE(object.hello.name(), "hello");
        QCOMPARE(object.world.name(), "world");

        SHOW(object.hello.features().value);
        SHOW(object.world.features().value);

        static_assert( object.hello.isReadable());
        static_assert(!object.hello.isResetable());
        static_assert(!object.hello.isNotifiable());
        static_assert(!object.hello.isWritable());

        static_assert( object.world.isReadable());
        static_assert(!object.world.isResetable());
        static_assert( object.world.isNotifiable());
        static_assert( object.world.isWritable());

        SHOW(metaObject.property(1).read(&object));
        SHOW(metaObject.property(2).read(&object));

        connect(&object, object.world.notifyPointer(), this, [](int newValue) {
            qInfo("World has changed to %d...", newValue);
        });

        object.world.connect(this, [](int newValue) {
            qInfo("The world totally has changed to %d...", newValue);
        });

        object.world = 13;

        using nproperty::detail::Prototype;

        // The objects returned by Prototype::get() are uninitialized, unconstructed data.
        // The test will randomly crash when QCOMPARE() doesn't try to read metaObject(),
        // and the like.

        QCOMPARE(reinterpret_cast<quintptr>(Prototype::get<HelloWorld>()),
                 reinterpret_cast<quintptr>(Prototype::get<HelloWorld>()));

        QCOMPARE(reinterpret_cast<quintptr>(Prototype::get<HelloWorld>()),
                 reinterpret_cast<quintptr>(Prototype::get<NObjectMacro>()));

        QCOMPARE(reinterpret_cast<quintptr>(Prototype::get(&HelloWorld::hello)),
                 reinterpret_cast<quintptr>(Prototype::get(&HelloWorld::hello)));

        QVERIFY(reinterpret_cast<quintptr>(Prototype::get(&HelloWorld::hello))
             != reinterpret_cast<quintptr>(Prototype::get(&HelloWorld::world)));
    }

private:

    /// --------------------------------------------------------------------------------------------
    /// An initial test for the most basic properties of the generated QMetaObjects
    /// --------------------------------------------------------------------------------------------

    template <HasFeature<MetaObject> T>
    static void testMetaObject(T &object)
    {
        const auto metaType = QMetaType::fromType<T>();
        const auto metaObject = T::staticMetaObject;

        static_assert(QtPrivate::HasQ_OBJECT_Macro<T>::Value);
        static_assert(std::is_convertible_v<decltype(metaObject), QMetaObject>);

        SHOW(sizeof(object));
        SHOW(sizeof(QString));

        if constexpr (isDataMember<&T::constant>)
            QCOMPARE(sizeof(T::constant), sizeof(QString));
        if constexpr (isDataMember<&T::notifying>)
            SHOW(sizeof(T::notifying));
        if constexpr (isDataMember<&T::writable>)
            SHOW(sizeof(T::writable));

        const QFETCH(QByteArray, expectedClassName);

        QVERIFY(metaObject.d.data);
        QVERIFY(metaObject.className() != nullptr);
        QVERIFY(metaObject.superClass() != nullptr);

        QCOMPARE(metaType.name(), expectedClassName);
        QCOMPARE(metaObject.className(), expectedClassName);
        QCOMPARE(metaObject.superClass()->className(), "experiment::ParentClass");
        QCOMPARE(metaObject.propertyCount(), 4);
        QCOMPARE(metaObject.methodCount(), 7);
    }

    static void testMetaObject(MObjectTest &object)
    {
        testMetaObject<MObjectTest>(object);

        QCOMPARE(sizeof(object.constant),           sizeof(QString));
        QCOMPARE(sizeof(object.notifying),          sizeof(QString));
        QCOMPARE(sizeof(object.writable),           sizeof(QString));
        QCOMPARE(sizeof(object.notifyingChanged),      sizeof(char));
    }

    /// --------------------------------------------------------------------------------------------
    /// Verify that the generated properties look like expected.
    /// --------------------------------------------------------------------------------------------

    template <HasFeature<PropertyDefinitions> T>
    static void testPropertyDefinitions(T &)
    {
        const auto metaObject = T::staticMetaObject;

        QVERIFY(metaObject.d.data);

        const auto offset     = metaObject.propertyOffset();
        const auto constant   = metaObject.property(offset + 0);
        const auto notifying  = metaObject.property(offset + 1);
        const auto writable   = metaObject.property(offset + 2);

        QCOMPARE(offset, QObject::staticMetaObject.propertyCount());

        QVERIFY (constant.isValid());
        QCOMPARE(constant.name(),          "constant");
        QCOMPARE(constant.typeName(),       "QString");
        QCOMPARE(constant.isReadable(),          true);
        QCOMPARE(constant.isWritable(),         false);
        QCOMPARE(constant.isResettable(),       false);
        QCOMPARE(constant.isDesignable(),        true);
        QCOMPARE(constant.isScriptable(),        true);
        QCOMPARE(constant.isStored(),            true);
        QCOMPARE(constant.isUser(),             false);
        QCOMPARE(constant.isConstant(),          true);
        QCOMPARE(constant.isFinal(),             true);
        QCOMPARE(constant.isRequired(),         false);
        QCOMPARE(constant.isBindable() ,        false);
        QCOMPARE(constant.isFlagType(),         false);
        QCOMPARE(constant.isEnumType(),         false);
        QCOMPARE(constant.hasNotifySignal(),    false);
        QCOMPARE(constant.revision(),               0);
        QCOMPARE(constant.hasStdCppSet(),       false); // QTBUG-120378
        QCOMPARE(constant.isAlias(),            false);

        QVERIFY (notifying.isValid());
        QCOMPARE(notifying.name(),        "notifying");
        QCOMPARE(notifying.typeName(),      "QString");
        QCOMPARE(notifying.isReadable(),         true);
        QCOMPARE(notifying.isWritable(),        false);
        QCOMPARE(notifying.isConstant(),        false);
        QCOMPARE(notifying.hasNotifySignal(),    true);
        QCOMPARE(notifying.hasStdCppSet(),      false); // QTBUG-120378

        QVERIFY (writable.isValid());
        QCOMPARE(writable.name(),          "writable");
        QCOMPARE(writable.typeName(),       "QString");
        QCOMPARE(writable.isReadable(),          true);
        QCOMPARE(writable.isWritable(),          true);
        QCOMPARE(writable.isConstant(),         false);
        QCOMPARE(writable.hasNotifySignal(),     true);
        QCOMPARE(writable.hasStdCppSet(),        true);
    }

    /// --------------------------------------------------------------------------------------------
    /// Verify that properties have unique ids if the type system needs this.
    /// --------------------------------------------------------------------------------------------

    template <HasFeature<UniquePropertyIds> T>
    static void testUniquePropertyIds(T &object)
    {
        static_assert(std::is_unsigned_v<decltype(object.constant.address())>);
        static_assert(std::is_unsigned_v<decltype(object.constant.offset())>);

        const auto uniqueIds = QSet{object. constant.label(),
                                    object.notifying.label(),
                                    object. writable.label()};

        QCOMPARE(uniqueIds.size(), 3);

        const auto uniqueOffsets = QSet{object. constant.offset(),
                                        object.notifying.offset(),
                                        object. writable.offset()};

        QCOMPARE(uniqueOffsets.size(), 3);

        const auto uniqueAddresses = QSet{object. constant.address(),
                                          object.notifying.address(),
                                          object. writable.address()};

        QCOMPARE(uniqueAddresses.size(), 3);
    }

    /// --------------------------------------------------------------------------------------------
    /// Verify that properties have reasonable addresses they can use their signals.
    /// --------------------------------------------------------------------------------------------

    template <HasFeature<PropertyAddresses> T>
    static void testPropertyAddresses(T &object)
    {
        const auto objectAddress = reinterpret_cast<quintptr>(&object);

        QCOMPARE_LT(object.notifying.offset(), sizeof(object));
        QCOMPARE_LT(object. writable.offset(), sizeof(object));

        QCOMPARE_GE(object. constant.address(), objectAddress);
        QCOMPARE_GE(object.notifying.address(), objectAddress);
        QCOMPARE_GE(object. writable.address(), objectAddress);

        QCOMPARE_LT(object. constant.address(), objectAddress + sizeof(object));
        QCOMPARE_LT(object.notifying.address(), objectAddress + sizeof(object));
        QCOMPARE_LT(object. writable.address(), objectAddress + sizeof(object));

        QCOMPARE(object. constant.offset() + objectAddress, object. constant.address());
        QCOMPARE(object.notifying.offset() + objectAddress, object.notifying.address());
        QCOMPARE(object. writable.offset() + objectAddress, object. writable.address());

        QCOMPARE(object. constant.object(), &object);
        QCOMPARE(object.notifying.object(), &object);
        QCOMPARE(object. writable.object(), &object);
    }

    /// --------------------------------------------------------------------------------------------
    /// Verify that the generated methods look like expected.
    /// --------------------------------------------------------------------------------------------

    template <HasFeature<MethodDefinitions> T>
    static void testMethodDefinitions(T &)
    {
        const auto metaObject = T::staticMetaObject;

        QVERIFY(metaObject.d.data);

        const auto offset            = metaObject.methodOffset();
        const auto notifyingChanged  = metaObject.method(offset + 0);
        const auto writableChanged   = metaObject.method(offset + 1);

        QCOMPARE(offset, QObject::staticMetaObject.methodCount());

        QVERIFY (notifyingChanged.isValid());
        QCOMPARE(notifyingChanged.name(),                     "notifyingChanged");
        QCOMPARE(notifyingChanged.typeName(),                             "void");
        QCOMPARE(notifyingChanged.methodSignature(), "notifyingChanged(QString)");
        QCOMPARE(notifyingChanged.methodType(),  QMetaMethod::MethodType::Signal);
        QCOMPARE(notifyingChanged.access(),          QMetaMethod::Access::Public);
        QCOMPARE(notifyingChanged.isConst(),                               false);
        QCOMPARE(notifyingChanged.revision(),                                  0);
        QCOMPARE(notifyingChanged.tag(),                                      "");
        QCOMPARE(notifyingChanged.parameterCount(),                            1);
        QCOMPARE(notifyingChanged.parameterTypeName(0),                "QString");
        QCOMPARE(notifyingChanged.parameterNames().at(0),            "notifying");

        QVERIFY (writableChanged.isValid());
        QCOMPARE(writableChanged.name(),                       "writableChanged");
        QCOMPARE(writableChanged.typeName(),                              "void");
        QCOMPARE(writableChanged.methodSignature(),   "writableChanged(QString)");
        QCOMPARE(writableChanged.methodType(),   QMetaMethod::MethodType::Signal);
        QCOMPARE(writableChanged.access(),           QMetaMethod::Access::Public);
        QCOMPARE(writableChanged.isConst(),                                false);
        QCOMPARE(writableChanged.revision(),                                   0);
        QCOMPARE(writableChanged.tag(),                                       "");
        QCOMPARE(writableChanged.parameterCount(),                             1);
        QCOMPARE(writableChanged.parameterTypeName(0),                 "QString");
        QCOMPARE(writableChanged.parameterNames().at(0),              "writable");
    }

    /// --------------------------------------------------------------------------------------------
    /// Verify that signals have reasonable addresses for QObject::connect().
    /// --------------------------------------------------------------------------------------------

    template <HasFeature<SignalAddresses> T>
    static void testSignalAddresses(T &)
    {
        QVERIFY(&T::notifyingChanged != &T::writableChanged);
    }

    /// --------------------------------------------------------------------------------------------
    /// Verify that notify pointers of roperties are correct
    /// --------------------------------------------------------------------------------------------

    template <HasFeature<NotifyPointers> T>
    static void testNotifyPointers(T &object)
    {
        QCOMPARE(object.notifying.notifyPointer(), &object.notifyingChanged);
        QCOMPARE(object.writable .notifyPointer(), &object. writableChanged);

        QCOMPARE(object.notifyingChanged.get(),    &object.notifyingChanged);
        QCOMPARE(object.writableChanged .get(),    &object. writableChanged);

        QCOMPARE(object.notifying.notifyPointer(),  object.notifyingChanged.get());
        QCOMPARE(object.writable .notifyPointer(),  object. writableChanged.get());
    }

    /// --------------------------------------------------------------------------------------------
    /// Verify that properties can be changed, and read
    /// --------------------------------------------------------------------------------------------

    template <HasFeature<PropertyChanges> T>
    static void testPropertyChanges(T &object)
    {
        QCOMPARE(object.constant(),             constant1);
        QCOMPARE(object.property("constant"),   constant1);

        QCOMPARE(object.notifying(),            notifying1);
        QCOMPARE(object.property("notifying"),  notifying1);

        QCOMPARE(object.writable(),             writable1);
        QCOMPARE(object.property("writable"),   writable1);

        object.modifyNotifying();

        QCOMPARE(object.notifying(),            notifying2);
        QCOMPARE(object.property("notifying"),  notifying2);

        object.setWritable(writable2);

        QCOMPARE(object.writable(),             writable2);
        QCOMPARE(object.property("writable"),   writable2);

        object.setProperty("notifying",         metacall1);

        QCOMPARE(object.notifying(),            notifying2);
        QCOMPARE(object.property("notifying"),  notifying2);

        object.setProperty("writable",          metacall1);

        QCOMPARE(object.writable(),             metacall1);
        QCOMPARE(object.property("writable"),   metacall1);
    }

    static void testPropertyChanges(MObjectTest &object)
    {
        testPropertyChanges<MObjectTest>(object);

        // object.constant = u"error"_s;    // compile error
        // object.notifying = u"error"_s;   // FIXME: currently would work, but shouldn't

        object.writable = writable3;

        QCOMPARE(object.writable(),             writable3);
        QCOMPARE(object.property("writable"),   writable3);
    }

    /// --------------------------------------------------------------------------------------------
    /// Verify that properties can be changed, and emit the expected signals.
    /// --------------------------------------------------------------------------------------------

    template <HasFeature<PropertyNotifications> T>
    static void testPropertyNotifications(T &object)
    {
        const auto metaObject = T::staticMetaObject;

        QVERIFY(metaObject.d.data);

        const auto notifyingChanged = QMetaMethod::fromSignal(&T::notifyingChanged);
        const auto writableChanged  = QMetaMethod::fromSignal(&T::writableChanged);

        QVERIFY (notifyingChanged.isValid());
        QCOMPARE(notifyingChanged.name(),                   "notifyingChanged");
        QCOMPARE(notifyingChanged.methodIndex(), metaObject.methodOffset() + 0);

        QVERIFY (writableChanged.isValid());
        QCOMPARE(writableChanged.name(),                    "writableChanged");
        QCOMPARE(writableChanged.methodIndex(), metaObject.methodOffset() + 1);

        auto notifyingSpy = QSignalSpy{&object, &T::notifyingChanged};
        auto writableSpy  = QSignalSpy{&object, &T::writableChanged};

        QVERIFY(notifyingSpy.isValid());
        QVERIFY(writableSpy. isValid());

        testPropertyNotifications(object, notifyingSpy, writableSpy);
    }

    template <class T>
    static void testPropertyNotifications(T          &object,
                                          QSignalSpy &notifyingSpy,
                                          QSignalSpy &writableSpy)
    {
        QCOMPARE(object.constant(),             constant1);
        QCOMPARE(object.property("constant"),   constant1);

        QCOMPARE(object.notifying(),            notifying1);
        QCOMPARE(object.property("notifying"),  notifying1);

        QCOMPARE(notifyingSpy,                  notifyingSpy1);
        QCOMPARE(writableSpy,                   writableSpy1);

        QCOMPARE(object.writable(),             writable1);
        QCOMPARE(object.property("writable"),   writable1);

        QCOMPARE(notifyingSpy,                  notifyingSpy1);
        QCOMPARE(writableSpy,                   writableSpy1);

        object.modifyNotifying();

        QCOMPARE(object.notifying(),            notifying2);
        QCOMPARE(object.property("notifying"),  notifying2);

        QCOMPARE(notifyingSpy,                  notifyingSpy2);
        QCOMPARE(writableSpy,                   writableSpy1);

        object.setWritable(writable2);

        QCOMPARE(object.writable(),             writable2);
        QCOMPARE(object.property("writable"),   writable2);

        QCOMPARE(notifyingSpy,                  notifyingSpy2);
        QCOMPARE(writableSpy,                   writableSpy2);

        object.setProperty("notifying",         metacall1);

        QCOMPARE(object.notifying(),            notifying2);
        QCOMPARE(object.property("notifying"),  notifying2);

        QCOMPARE(notifyingSpy,                  notifyingSpy2);
        QCOMPARE(writableSpy,                   writableSpy2);

        object.setProperty("writable",          metacall1);

        QCOMPARE(object.writable(),             metacall1);
        QCOMPARE(object.property("writable"),   metacall1);

        QCOMPARE(notifyingSpy,                  notifyingSpy2);
        QCOMPARE(writableSpy,                   writableSpy3);
    }

    static void testPropertyNotifications(MObjectTest &object,
                                          QSignalSpy  &notifyingSpy,
                                          QSignalSpy  &writableSpy)
    {
        testPropertyNotifications<MObjectTest>(object, notifyingSpy, writableSpy);

        // object.constant = u"error"_s;    // compile error
        // object.notifying = u"error"_s;   // FIXME: currently would work, but shouldn't

        object.writable = writable3;

        QCOMPARE(object.writable(),             writable3);
        QCOMPARE(object.property("writable"),   writable3);

        QCOMPARE(writableSpy,                   writableSpy4);
    }

    /// --------------------------------------------------------------------------------------------
    /// Verify that classinfo is generated
    /// --------------------------------------------------------------------------------------------

    template <HasFeature<ClassInfo> T>
    static void testClassInfo(T &)
    {
        const QMetaObject metaObject = T::staticMetaObject;

        QCOMPARE(metaObject.classInfoCount(), 1);
        QCOMPARE(metaObject.classInfoOffset(), 0);
        QCOMPARE(metaObject.classInfo(0).name(), "URL");
        QCOMPARE(metaObject.classInfo(0).value(), PROJECT_HOMEPAGE_URL);
    }

    /// --------------------------------------------------------------------------------------------
    /// Verify that interfaces are supported
    /// --------------------------------------------------------------------------------------------

    template <HasFeature<Interfaces> T>
    static void testInterfaces(T &object)
    {
        QCOMPARE(qobject_interface_iid<experiment::InterfaceOne *>(),
                                       "experiment/InterfaceOne/1.0");
        QCOMPARE(qobject_interface_iid<experiment::InterfaceTwo *>(),
                                       "experiment/InterfaceTwo/1.0");

        QVERIFY(qobject_cast<QObject                  *>(&object));
        QVERIFY(qobject_cast<experiment::ParentClass  *>(&object));
        QVERIFY(qobject_cast<experiment::InterfaceOne *>(&object));
        QVERIFY(qobject_cast<experiment::InterfaceTwo *>(&object));

        QCOMPARE(qobject_cast<QObject                  *>(&object),
                  static_cast<QObject                  *>(&object));
        QCOMPARE(qobject_cast<experiment::ParentClass  *>(&object),
                  static_cast<experiment::ParentClass  *>(&object));
        QCOMPARE(qobject_cast<experiment::InterfaceOne *>(&object),
                  static_cast<experiment::InterfaceOne *>(&object));
        QCOMPARE(qobject_cast<experiment::InterfaceTwo *>(&object),
                  static_cast<experiment::InterfaceTwo *>(&object));

        QCOMPARE(object.qt_metacast("QObject"),
                         static_cast<QObject *>(&object));
        QCOMPARE(object.qt_metacast("experiment::ParentClass"),
                         static_cast<experiment::ParentClass  *>(&object));
        QCOMPARE(object.qt_metacast("experiment::InterfaceOne"),
                         static_cast<experiment::InterfaceOne *>(&object));
        QCOMPARE(object.qt_metacast("experiment::InterfaceTwo"),
                         static_cast<experiment::InterfaceTwo *>(&object));

        QCOMPARE(object.qt_metacast("experiment/InterfaceOne/1.0"),
                 static_cast<experiment::InterfaceOne *>(&object));
        QCOMPARE(object.qt_metacast("experiment/InterfaceTwo/1.0"),
                 static_cast<experiment::InterfaceTwo *>(&object));

        QCOMPARE(object.qt_metacast(object.metaObject()->className()), &object);
        QCOMPARE(object.qt_metacast("nonsense"),                       nullptr);
        QCOMPARE(object.qt_metacast(nullptr),                          nullptr);

        QCOMPARE(qobject_cast<experiment::InterfaceOne *>(&object)
                     -> firstInterfaceCall(),  "first");
        QCOMPARE(qobject_cast<experiment::InterfaceTwo *>(&object)
                     ->secondInterfaceCall(), "second");
    }

    /// --------------------------------------------------------------------------------------------
    /// Verify that enumerators are generated
    /// --------------------------------------------------------------------------------------------

    template <HasFeature<Enumerators> T>
    static void testEnumerators(T &)
    {
#ifdef NPROPERTY_NMETAENUM_DEBUG
        qInfo() << nproperty::metaenum::debug::samples();
#endif // NPROPERTY_NMETAENUM_DEBUG

        static_assert(!nproperty::ScopedEnumType<typename T::Error>);
        static_assert( nproperty::ScopedEnumType<typename T::Option>);

        static_assert(QtPrivate::IsQEnumHelper<typename T::Error>::Value);
        static_assert(QtPrivate::IsQEnumHelper<typename T::Option>::Value);

        const QMetaObject &metaObject = T::staticMetaObject;

        QCOMPARE(metaObject.enumeratorCount(), 2);
        QCOMPARE(metaObject.enumeratorOffset(), 0);

        const auto errorEnumFromMeta = metaObject.enumerator(0);
        const auto errorEnumFromType = QMetaEnum::fromType<typename T::Error>();

        QVERIFY (errorEnumFromMeta.isValid());
        QCOMPARE(errorEnumFromMeta.name(),                    "Error");
        QCOMPARE(errorEnumFromMeta.enumName(),                "Error");
        QCOMPARE(errorEnumFromMeta.isFlag(),                    false);
        QCOMPARE(errorEnumFromMeta.isScoped(),                  false);
        QCOMPARE(errorEnumFromMeta.keyCount(),                      2);
        QCOMPARE(errorEnumFromMeta.key(0),                  "NoError");
        QCOMPARE(errorEnumFromMeta.key(1),                "SomeError");
        QCOMPARE(errorEnumFromMeta.value(0),               T::NoError);
        QCOMPARE(errorEnumFromMeta.value(1),             T::SomeError);
        QCOMPARE(errorEnumFromMeta.value(0),        T::Error::NoError);
        QCOMPARE(errorEnumFromMeta.value(1),      T::Error::SomeError);
        QCOMPARE(errorEnumFromMeta.scope(),    metaObject.className());
        QCOMPARE(errorEnumFromMeta.enclosingMetaObject(), &metaObject);

        QCOMPARE(errorEnumFromMeta.enclosingMetaObject(),
                 errorEnumFromType.enclosingMetaObject());

        const auto optionEnumFromMeta = metaObject.enumerator(1);
        const auto optionEnumFromType = QMetaEnum::fromType<typename T::Option>();

        QVERIFY (optionEnumFromMeta.isValid());
        QCOMPARE(optionEnumFromMeta.name(),                              "Option");
        QCOMPARE(optionEnumFromMeta.enumName(),                          "Option");
        QCOMPARE(optionEnumFromMeta.isFlag(),                                true);
        QCOMPARE(optionEnumFromMeta.isScoped(),                              true);
        QCOMPARE(optionEnumFromMeta.keyCount(),                                 2);
        QCOMPARE(optionEnumFromMeta.key(0),                               "First");
        QCOMPARE(optionEnumFromMeta.key(1),                              "Second");
        QCOMPARE(optionEnumFromMeta.value(0),  static_cast<int>(T::Option::First));
        QCOMPARE(optionEnumFromMeta.value(1), static_cast<int>(T::Option::Second));
        QCOMPARE(optionEnumFromMeta.scope(),               metaObject.className());
        QCOMPARE(optionEnumFromMeta.enclosingMetaObject(),            &metaObject);

        QCOMPARE(optionEnumFromMeta.enclosingMetaObject(),
                 optionEnumFromType.enclosingMetaObject());

#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)

        QCOMPARE(errorEnumFromMeta.metaType(),
                 errorEnumFromType.metaType());
        QCOMPARE(optionEnumFromMeta.metaType(),
                 optionEnumFromType.metaType());

#endif
    }

    /// --------------------------------------------------------------------------------------------
    /// Some support functions to provide same test environment for all property systems.
    /// --------------------------------------------------------------------------------------------

    using TestFunction = void (*)();
    using TestFunctionPointer = void *;

    void runFeatureTest()
    {
        const QFETCH(TestFunctionPointer, testFunctionPointer);
        const auto testFunction = reinterpret_cast<TestFunction>(testFunctionPointer);

        if (testFunction == nullptr)
            QSKIP("Not implemented yet");

        testFunction();
    }

    void runBenchmark()
    {
        const QFETCH(TestFunctionPointer, testFunctionPointer);
        const auto testFunction = reinterpret_cast<TestFunction>(testFunctionPointer);

        if (testFunction == nullptr)
            QSKIP("Not implemented yet");

        QBENCHMARK {
            for (auto i = 0; i < 1000; ++i)
                testFunction();
        }
    }

    template<Feature feature, class Delegate>
    void makeTestData()
    {
        QTest::addColumn<TestFunctionPointer>("testFunctionPointer");
        QTest::addColumn<QByteArray>         ("expectedClassName");

        makeTestRow<feature, Delegate, AObjectTest>  ();
        makeTestRow<feature, Delegate, MObjectTest>  ();
        makeTestRow<feature, Delegate, NObjectMacro> ();
        makeTestRow<feature, Delegate, NObjectModern>();
        makeTestRow<feature, Delegate, NObjectLegacy>();
        makeTestRow<feature, Delegate, SObjectTest>  ();
    }

    template<Feature feature, typename Delegate, class T>
    void makeTestRow()
    {
        if constexpr (isImplementedFeature<feature, T>) {
            const auto testFunction = []() -> TestFunctionPointer {
                if constexpr (!isSkippedFeature<feature, T>) {
                    // GCC fails to wrap template function pointers by QVariant on
                    // Qt 6.2 for MacOS. Actually I am suprised that this worked at all.
                    // So let's just erase the function type to make live easier for GCC.
                    const auto testFunction = &PropertyExperiment::testFunction<Delegate, T>;
                    return reinterpret_cast<TestFunctionPointer>(testFunction);
                }

                return nullptr;
            }();

            const auto className = T::staticMetaObject.className();
            const auto tag = std::strrchr(className, ':') + 1;
            Q_ASSERT((tag - 1) != nullptr && tag[-1] == ':');

            QTest::newRow(tag) << testFunction << QByteArray{className};
        }
    }

    template<class Delegate, class T>
    static void testFunction()
    {
        auto object = T{};
        Delegate{}(object);
    }
};

} // namespace

QTEST_MAIN(PropertyExperiment)

#include "main.moc"
