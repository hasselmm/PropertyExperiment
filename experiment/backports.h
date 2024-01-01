#ifndef PROPERTYEXPERIMENT_BACKPORTS_H
#define PROPERTYEXPERIMENT_BACKPORTS_H

#include <QTest>

/// It's really helpful to also know the values of a failing test. Therefore
/// let's use them, but have primitive fallback versions on older versions of Qt.
///
#ifndef QCOMPARE_LT
#define QCOMPARE_LT(Left, Right) QVERIFY((Left) < (Right))
#define QCOMPARE_GE(Left, Right) QVERIFY((Left) >= (Right))
#endif

#if QT_VERSION < QT_VERSION_CHECK(6, 5, 0)

inline QDebug operator<<(QDebug debug, const std::string &s)
{
    return debug << QUtf8StringView{s};
}

inline QDebug operator<<(QDebug debug, const std::string_view &s)
{
    return debug << QUtf8StringView{s};
}

#endif

#endif // PROPERTYEXPERIMENT_BACKPORTS_H
