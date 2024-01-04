#ifndef REPORTING_REPORTING_H
#define REPORTING_REPORTING_H

#include <QDateTime>
#include <QString>

#include <map>
#include <ranges>
#include <vector>

namespace reporting {

struct Message
{
    enum class Type { Error, Fail, Skip, Pass };

    Message() noexcept = default;
    Message(QString type, QString file, QString line, QString tag, QString text) noexcept
        : type{std::move(type)}, file{std::move(file)}, line{std::move(line)}
        , text{std::move(text)},  tag{std::move(tag)}
    {}

    QString type;
    QString file;
    QString line;
    QString text;
    QString tag;
};

struct Benchmark
{
    QString tag;
    QString metric;
    QString value;
    QString iterations;
};

struct TestFunction
{
    QString                 name;
    QString                 duration;
    std::vector<Message>    messages;
    std::vector<Benchmark>  benchmarks;
};

struct TestReport
{
    QString                     name;
    QString                     hostname;
    std::map<QString, QString>  properties;
    std::vector<TestFunction>   functions;
    QString                     duration;
    QDateTime                   timestamp;
};

namespace functional {

[[nodiscard]] constexpr QStringView name(Message::Type type) noexcept
{
    switch (type) {
    case Message::Type::Error:
        return u"qfatal";
    case Message::Type::Fail:
        return u"fail";
    case Message::Type::Skip:
        return u"skip";
    case Message::Type::Pass:
        return u"pass";
    }

    return {};
}

[[nodiscard]] inline auto hasType(QStringView type) noexcept
{
    return [type](const auto &item) {
        return item.type == type;
    };
}

[[nodiscard]] inline auto hasType(Message::Type type) noexcept
{
    return hasType(name(type));
}

inline const auto isError = hasType(Message::Type::Error);
inline const auto isFail  = hasType(Message::Type::Fail);
inline const auto isPass  = hasType(Message::Type::Pass);
inline const auto isSkip  = hasType(Message::Type::Skip);

[[nodiscard]] inline auto messageCount(auto predicate) noexcept
{
    return [predicate](const auto &item) {
        return std::ranges::count_if(item.messages, predicate);
    };
}

[[nodiscard]] inline auto sum(const auto &range) noexcept
{
    return std::accumulate(std::cbegin(range), std::cend(range), 0);
}

[[nodiscard]] inline auto countErrors(const TestReport *report) noexcept
{
    return sum(report->functions | std::views::transform(messageCount(isError)));
}

[[nodiscard]] inline auto countFailures(const TestReport *report) noexcept
{
    return sum(report->functions | std::views::transform(messageCount(isFail)));
}

[[nodiscard]] inline auto countSkipped(const TestReport *report) noexcept
{
    return sum(report->functions | std::views::transform(messageCount(isSkip)));
}

[[nodiscard]] inline auto hasLocation(const Message &message) noexcept
{
    return !message.file.isEmpty() && !message.line.isEmpty();
}

[[nodiscard]] inline auto hasBenchmarks(const TestFunction &function)
{
    return !function.benchmarks.empty();
}

} // namespace functional
} // namespace reporting

#endif // REPORTING_REPORTING_H
