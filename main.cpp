#include "aobject.h"
#include "mobject.h"
#include "sobject.h"

#include <QSignalSpy>
#include <QTest>

static_assert(QtPrivate::HasQ_OBJECT_Macro<aproperty::AObject>::Value);
static_assert(QtPrivate::HasQ_OBJECT_Macro<mproperty::MObject>::Value);
static_assert(QtPrivate::HasQ_OBJECT_Macro<sproperty::SObject>::Value);

namespace {

#define SHOW(What) qInfo() << #What " =>" << (What)

class PropertyTest: public QObject
{
    Q_OBJECT

private:
    template <class T>
    static void testProperties()
    {
        const QFETCH(QByteArrayView, expectedClassName);
        const QFETCH(QByteArrayView, expectedSuperClassName);

        const auto typeName = QMetaType::fromType<T>().name();
        QCOMPARE(typeName, expectedClassName);

        auto object = T{};
        const auto &metaObject = object.staticMetaObject;

        SHOW(sizeof(object));

        if constexpr (!std::is_same_v<T, sproperty::SObject>) {
            SHOW(sizeof(object.constant));
            SHOW(sizeof(object.notifying));
            SHOW(sizeof(object.writable));
            SHOW(sizeof(QString));
        }

        QCOMPARE(metaObject.className(), typeName);
        QCOMPARE(metaObject.superClass()->className(), expectedSuperClassName);
        QCOMPARE(metaObject.propertyCount(), 4);
        QCOMPARE(metaObject.methodCount(), 7);

        const auto constant  = metaObject.property(1);
        const auto notifying = metaObject.property(2);
        const auto writable  = metaObject.property(3);

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
        QCOMPARE(constant.hasStdCppSet(),       false); // FIXME why is this false for AObject?
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

        if constexpr (std::is_same_v<T, mproperty::MObject>) {
            const auto objectAddress = reinterpret_cast<qintptr>(&object);

            QCOMPARE(object. constant.offset() + objectAddress, object. constant.address());
            QCOMPARE(object.notifying.offset() + objectAddress, object.notifying.address());
            QCOMPARE(object. writable.offset() + objectAddress, object. writable.address());

            QCOMPARE(object. constant.object(), &object);
            QCOMPARE(object.notifying.object(), &object);
            QCOMPARE(object. writable.object(), &object);

            QCOMPARE(object.notifyingChanged.get(), &object.notifyingChanged);
            QCOMPARE(object. writableChanged.get(), &object. writableChanged);
        }

        QVERIFY(&T::notifyingChanged != &T::writableChanged);

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

    using TestPropertiesFunction = void (*)();

private slots:
    void testProperties_data()
    {
        QTest::addColumn<TestPropertiesFunction>("testRoutine");
        QTest::addColumn<QByteArrayView>("expectedClassName");
        QTest::addColumn<QByteArrayView>("expectedSuperClassName");

        QTest::newRow("aproperty")
            << &PropertyTest::testProperties<aproperty::AObject>
            << QByteArrayView{"aproperty::AObject"}
            << QByteArrayView{"QObject"};

        QTest::newRow("sproperty")
            << &PropertyTest::testProperties<sproperty::SObject>
            << QByteArrayView{"sproperty::SObject"}
            << QByteArrayView{"QObject"};

        QTest::newRow("mproperty")
            << &PropertyTest::testProperties<mproperty::MObject>
            << QByteArrayView{"mproperty::MObject"}
            << QByteArrayView{"mproperty::MObjectBase"};
    }

    void testProperties()
    {
        const QFETCH(TestPropertiesFunction, testRoutine);
        testRoutine();
    }
};

} // namespace

QTEST_MAIN(PropertyTest)

#include "main.moc"
