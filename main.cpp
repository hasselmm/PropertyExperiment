#include "aobject/aobject.h"
#include "mobject/mobject.h"
#include "sobject/sobject.h"

#include <QSignalSpy>
#include <QTest>

namespace {

/// Just a tiny wrapper with simple name for the pretty verbose
/// `!std::is_member_function_pointer_v<decltype(&T::member)>`.
///
template<auto pointer>
constexpr bool isDataMember = std::is_member_pointer_v<decltype(pointer)>
                              && !std::is_member_function_pointer_v<decltype(pointer)>;

/// Simply show an expression and its value.
///
#define SHOW(What) qInfo() << #What " =>" << (What)

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
    void testMetaObject()                                           { runTestFunction(); }
    void testMetaObject_data()                     { makeTestData<MetaObjectDelegate>(); }

    void testPropertyDefinitions()                                  { runTestFunction(); }
    void testPropertyDefinitions_data()   { makeTestData<PropertyDefinitionsDelegate>(); }

    void testUniquePropertyIds()                                    { runTestFunction(); }
    void testUniquePropertyIds_data()       { makeTestData<UniquePropertyIdsDelegate>(); }

    void testPropertyAddresses()                                    { runTestFunction(); }
    void testPropertyAddresses_data()       { makeTestData<PropertyAddressesDelegate>(); }

    void testSignalAddresses()                                      { runTestFunction(); }
    void testSignalAddresses_data()           { makeTestData<SignalAddressesDelegate>(); }

    void testPropertyChanges()                                      { runTestFunction(); }
    void testPropertyChanges_data()           { makeTestData<PropertyChangesDelegate>(); }

private:

    /// --------------------------------------------------------------------------------------------
    /// An initial test for the most basic properties of the generated QMetaObjects
    /// --------------------------------------------------------------------------------------------

    template <class T>
    static void testMetaObject(T &object)
    {
        const auto metaType = QMetaType::fromType<T>();
        const auto metaObject = T::staticMetaObject;

        static_assert(QtPrivate::HasQ_OBJECT_Macro<T>::Value);
        static_assert(std::is_convertible_v<decltype(metaObject), QMetaObject>);

        SHOW(sizeof(object));

        const QFETCH(QByteArray, expectedClassName);
        const QFETCH(QByteArray, expectedSuperClassName);

        QCOMPARE(metaType.name(), expectedClassName);
        QCOMPARE(metaObject.className(), expectedClassName);
        QCOMPARE(metaObject.superClass()->className(), expectedSuperClassName);
        QCOMPARE(metaObject.propertyCount(), 4);
        QCOMPARE(metaObject.methodCount(), 7);
    }

    /// --------------------------------------------------------------------------------------------
    /// Verify that the generated properties look like expected.
    /// --------------------------------------------------------------------------------------------

    template <class T>
    static void testPropertyDefinitions(T &object)
    {
        const auto metaObject = T::staticMetaObject;
        const auto constant   = metaObject.property(1);
        const auto notifying  = metaObject.property(2);
        const auto writable   = metaObject.property(3);

        if constexpr (isDataMember<&T::constant>)
            QCOMPARE(sizeof(T::constant), sizeof(QString));
        if constexpr (isDataMember<&T::notifying>)
            SHOW(sizeof(T::notifying));
        if constexpr (isDataMember<&T::writable>)
            SHOW(sizeof(T::writable));

        SHOW(sizeof(QString));

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
        QCOMPARE(constant.hasStdCppSet(),       false); // FIXME why is this false for SObject?
        QCOMPARE(constant.isAlias(),            false);

        QVERIFY (notifying.isValid());
        QCOMPARE(notifying.name(),        "notifying");
        QCOMPARE(notifying.typeName(),      "QString");
        QCOMPARE(notifying.isReadable(),         true);
        QCOMPARE(notifying.isWritable(),        false);
        QCOMPARE(notifying.isConstant(),        false);
        QCOMPARE(notifying.hasNotifySignal(),    true);

        QVERIFY (writable.isValid());
        QCOMPARE(writable.name(),          "writable");
        QCOMPARE(writable.typeName(),       "QString");
        QCOMPARE(writable.isReadable(),          true);
        QCOMPARE(writable.isWritable(),          true);
        QCOMPARE(writable.isConstant(),         false);
        QCOMPARE(writable.hasNotifySignal(),     true);
    }

    /// --------------------------------------------------------------------------------------------
    /// Verify that properties have unique ids if the type system needs this.
    /// --------------------------------------------------------------------------------------------

    template <class T>
    static void testUniquePropertyIds(T &)
    {}

    static void testUniquePropertyIds(mproperty::MObject &object)
    {
        const auto uniqueIds = QSet{object. constant.uniqueId(),
                                    object.notifying.uniqueId(),
                                    object. writable.uniqueId()};

        QCOMPARE(uniqueIds.size(), 3);

        const auto uniqueOffsets = QSet{object. constant.offset(),
                                        object.notifying.offset(),
                                        object. writable.offset()};

        QCOMPARE(uniqueOffsets.size(), 3);

        const auto uniqueAddresses = QSet{object. constant.address(),
                                          object.notifying.address(),
                                          object. writable.address()};

        QCOMPARE(uniqueAddresses.size(), 3);

        testUniquePropertyIds<mproperty::MObject>(object);
    }

    /// --------------------------------------------------------------------------------------------
    /// Verify that properties have reasonable addresses they can use their signals.
    /// --------------------------------------------------------------------------------------------

    template <class T>
    static void testPropertyAddresses(T &)
    {}

    static void testPropertyAddresses(mproperty::MObject &object)
    {
        const auto objectAddress = reinterpret_cast<qintptr>(&object);

        QCOMPARE(object. constant.offset() + objectAddress, object. constant.address());
        QCOMPARE(object.notifying.offset() + objectAddress, object.notifying.address());
        QCOMPARE(object. writable.offset() + objectAddress, object. writable.address());

        QCOMPARE(object. constant.object(), &object);
        QCOMPARE(object.notifying.object(), &object);
        QCOMPARE(object. writable.object(), &object);

        testPropertyAddresses<mproperty::MObject>(object);
    }

    /// --------------------------------------------------------------------------------------------
    /// Verify that signals have reasonable addresses for QObject::connect().
    /// --------------------------------------------------------------------------------------------

    template <class T>
    static void testSignalAddresses(T &)
    {
        QVERIFY(&T::notifyingChanged != &T::writableChanged);
    }

    static void testSignalAddresses(mproperty::MObject &object)
    {
        QCOMPARE(object.notifyingChanged.get(), &object.notifyingChanged);
        QCOMPARE(object. writableChanged.get(), &object. writableChanged);

        testSignalAddresses<mproperty::MObject>(object);
    }

    /// --------------------------------------------------------------------------------------------
    /// Verify that properties can be changed, and emit the expected signals.
    /// --------------------------------------------------------------------------------------------

    template <class T>
    static void testPropertyChanges(T &object)
    {
        auto notifyingSpy = QSignalSpy{&object, &T::notifyingChanged};
        auto writableSpy  = QSignalSpy{&object, &T::writableChanged};

        const auto  constant1 = u"I am constant"_qs;
        const auto notifying1 = u"I am observing"_qs;
        const auto notifying2 = u"I have been changed per method"_qs;
        const auto  writable1 = u"I am modifiable"_qs;
        const auto  writable2 = u"I have been changed per setter"_qs;
        const auto  metacall1 = u"I have been changed per metacall"_qs;

        const auto notifyingSpy1 = QList<QVariantList>{};
        const auto notifyingSpy2 = QList<QVariantList>{{notifying2}};

        const auto writableSpy1  = QList<QVariantList>{};
        const auto writableSpy2  = QList<QVariantList>{{writable2}};
        const auto writableSpy3  = QList<QVariantList>{{writable2}, {metacall1}};

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

        if constexpr (std::is_same_v<T, mproperty::MObject>) {
            auto uniqueIds = QSet{object.constant. uniqueId(),
                                  object.notifying.uniqueId(),
                                  object.writable. uniqueId()};

            QCOMPARE(uniqueIds.size(), 3);

            QCOMPARE(sizeof(object.constant),           sizeof(QString));
            QCOMPARE(sizeof(object.notifying),          sizeof(QString));
            QCOMPARE(sizeof(object.writable),           sizeof(QString));
            QCOMPARE(sizeof(object.notifyingChanged),      sizeof(char));

            const auto writable3     = u"I have been changed by assignment"_qs;
            const auto writableSpy3  = QList<QVariantList>{{writable2}, {metacall1}, {writable3}};

            // object.constant = u"error"_s;    // compile error
            // object.notifying = u"error"_s;   // FIXME: currently would work, but shouldn't

            object.writable = writable3;

            QCOMPARE(object.writable(),             writable3);
            QCOMPARE(object.property("writable"),   writable3);

            QCOMPARE(writableSpy,                   writableSpy3);
        }
    }

    static void testPropertyChanges(mproperty::MObject &object)
    {
        testPropertyChanges<mproperty::MObject>(object);
        qInfo("BAR");
    }

    /// --------------------------------------------------------------------------------------------
    /// Some support functions to provide same test environment for all property systems.
    /// --------------------------------------------------------------------------------------------

    void runTestFunction()
    {
        const QFETCH(TestFunction, testFunction);
        testFunction();
    }

    using TestFunction = void (*)();

    struct NullDelegate
    {
        static void run(auto &) {}
    };

#define DEFINE_FUNCTION_DELEGATE(Name)                                  \
    struct Name##Delegate                                               \
    {                                                                   \
        static void run(auto &o) { PropertyExperiment::test##Name(o); } \
    };

    DEFINE_FUNCTION_DELEGATE(MetaObject)
    DEFINE_FUNCTION_DELEGATE(PropertyDefinitions)
    DEFINE_FUNCTION_DELEGATE(UniquePropertyIds)
    DEFINE_FUNCTION_DELEGATE(PropertyAddresses)
    DEFINE_FUNCTION_DELEGATE(SignalAddresses)
    DEFINE_FUNCTION_DELEGATE(PropertyChanges)

#undef DEFINE_FUNCTION_DELEGATE

    template<class FunctionDelegate = NullDelegate>
    void makeTestData(const NullDelegate * = nullptr)
    {
        QTest::addColumn<TestFunction>  ("testFunction");
        QTest::addColumn<QByteArray>    ("expectedClassName");
        QTest::addColumn<QByteArray>    ("expectedSuperClassName");

        makeTestRow<FunctionDelegate, aproperty::AObject>
            ("aproperty", "aproperty::AObject");
        makeTestRow<FunctionDelegate, mproperty::MObject>
            ("mproperty", "mproperty::MObject", "mproperty::MObjectBase");
        makeTestRow<FunctionDelegate, sproperty::SObject>
            ("sproperty", "sproperty::SObject");
    }

    template<class FunctionDelegate, class T>
    void makeTestRow(const char *tag, QByteArray className,
                     QByteArray superName = "QObject")
    {
        const auto function = [] {
            auto object = T{};
            FunctionDelegate::run(object);
        };

        QTest::newRow(tag)
            << static_cast<TestFunction>(function)
            << className << superName;
    }
};

} // namespace

QTEST_MAIN(PropertyExperiment)

#include "main.moc"
