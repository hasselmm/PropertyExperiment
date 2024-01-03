#include "markdownwriter.h"

#include <QFile>
#include <QLoggingCategory>

#include <ranges>
#include <set>

namespace reporting {
namespace {

Q_LOGGING_CATEGORY(lcMarkdownWriter, "reporting.markdownwriter")

using Qt::endl;

auto bold(QStringView text)
{
    return u"**"_qs + text.toString() + u"**"_qs;
}

auto length(QStringView text)
{
    return text.length();
}

auto maximumLength(const auto &range)
{
    return std::ranges::max(range | std::views::transform(length));
}

template<typename T>
using Categorized = std::pair<std::pair<QString, QString>, const T *>;

auto categorize(const TestFunction &function)
{
    const auto name = std::make_pair(function.name.section(u':', 0, -3),
                                     function.name.section(u':', -1));
    return std::make_pair(name, std::addressof(function));
}

template<typename T>
auto category(const Categorized<T> &p)
{
    return p.first.first;
}

template<typename T>
auto function(const Categorized<T> &p)
{
    return p.first.second;
}

template<std::forward_iterator Iterator, typename T = typename Iterator::value_type>
std::vector<T> uniqueValues(Iterator first, Iterator last)
{
    auto values = std::vector<T>{first, last};

    std::ranges::sort(values);
    const auto tail = std::ranges::unique(values);
    values.erase(tail.begin(), tail.end());

    return values;
}

auto uniqueValues(auto &&range)
{
    return uniqueValues(std::begin(range), std::end(range));
}

auto makeUniqueCategories(auto &&range)
{
    return uniqueValues(std::move(range) | std::views::transform(category<TestFunction>));
}

auto makeUniqueFunctionNames(auto &&range)
{
    return uniqueValues(std::move(range) | std::views::transform(function<TestFunction>));
}

auto makeCategoryTitles(const TestReport *report, const std::vector<QString> &categories)
{
    const auto makeTitle = [report](QString category) {
        const auto qtVersion = report->properties.at(category + u"::QtVersion"_qs);
        const auto compiler = report->properties.at(category + u"::Compiler"_qs);
        const auto config = report->properties.at(category + u"::CMakeConfig"_qs);
        auto title = compiler + u" Qt\u00a0"_qs + qtVersion + u' ' + config;
        return std::make_pair(std::move(category), std::move(title));
    };

    const auto categoryTitlePairs = categories | std::views::transform(makeTitle);
    return std::map{categoryTitlePairs.begin(), categoryTitlePairs.end()};
}

} // namespace

bool writeMarkdownBenchmarkReport(const TestReport *report, QIODevice *device)
{
    const auto extractDataTag = [](const QString &functionName) {
        if (const auto i = functionName.indexOf(u'(');
            i > 0 && functionName.endsWith(u')')) {
            auto dataTag = functionName.mid(i + 1, functionName.length() - i  - 2);
            return std::make_tuple(functionName, functionName.left(i - 1), std::move(dataTag));
        } else {
            return std::make_tuple(functionName, functionName, QString{});
        }
    };

    auto benchmarks = report->functions
                      | std::views::filter(functional::hasBenchmarks)
                      | std::views::transform(categorize);

    const auto results              = std::map{benchmarks.begin(), benchmarks.end()};
    const auto categories           = makeUniqueCategories(benchmarks);
    const auto categoryTitles       = makeCategoryTitles(report, categories);
    const auto uniqueBenchmarkNames = makeUniqueFunctionNames(benchmarks);
    const auto taggedBenchmarkNames = uniqueBenchmarkNames | std::views::transform(extractDataTag);

    const auto measurement = [&results](const auto &cn, const auto &bn) {
        const auto it = results.find({cn, bn});

        if (it == results.cend())
            return QString{};

        return it->second->benchmarks.front().value;
    };

    auto stream = QTextStream{device};

    stream << "# Benchmark Results" << endl;
    stream << endl;

    enum { BenchmarkId, BenchmarkName, DataTag };

    for (auto it = taggedBenchmarkNames.begin(); it != taggedBenchmarkNames.end(); ) {
        const auto benchmarkName = std::get<BenchmarkName>(*it);
        const auto isNextBenchmark = [benchmarkName](const auto &tuple) {
            return std::get<BenchmarkName>(tuple) != benchmarkName;
        };

        const auto next  = std::find_if(it, taggedBenchmarkNames.end(), isNextBenchmark);
        const auto first = std::exchange(it, next);
        const auto last  = it;

        stream << "## " << benchmarkName << endl;
        stream << endl;
        stream << "```mermaid" << endl;
        stream << "xychart-beta" << endl;
        stream << "  title \"" << benchmarkName << '"' << endl;
        stream << "  x-axis [";

        if (auto it = first; it != last) {
            stream << '"' << std::get<DataTag>(*it) << '"';

            while (++it != last)
                stream << ", \"" << std::get<DataTag>(*it) << '"';
        }

        stream << "]" << endl;
        stream << "  y-axis \"Duration in ms\"" << endl;

        for (const auto &cn : categories) {
            stream << "  line [";

            if (auto it = first; it != last) {
                stream << measurement(cn, std::get<BenchmarkId>(*it));

                while (++it != last)
                    stream << ", " << measurement(cn, std::get<BenchmarkId>(*it));
            }

            stream << "]" << endl;
        }

        stream << "```" << endl;
        stream << endl;
    }

    for (const auto &cn : categories)
        stream << "* " << cn << endl;

    return true;
}

bool writeMarkdownBenchmarkReport(const TestReport *report, QStringView fileName)
{
    auto file = QFile{fileName.toString()};

    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        qCWarning(lcMarkdownWriter, "%ls: %ls",
                  qUtf16Printable(file.fileName()),
                  qUtf16Printable(file.errorString()));

        return false;
    }

    qCInfo(lcMarkdownWriter,
           R"(Writing benchmark results to "%ls"...)",
           qUtf16Printable(file.fileName()));

    return writeMarkdownBenchmarkReport(report, &file);
}

bool writeMarkdownTestReport(const TestReport *report, QIODevice *device)
{
    using enum Message::Type;

    const auto statuses = std::map<Message::Type, QString> {
        {Error, u":fire:  error"_qs},
        {Fail,  u":zap:   failed"_qs},
        {Skip,  u":ghost: skipped"_qs},
        {Pass,  u":heart: passed"_qs}
    };

    auto functions = report->functions | std::views::transform(categorize);

    const auto results        = std::map{functions.begin(), functions.end()};
    const auto categories     = makeUniqueCategories(functions);
    const auto categoryTitles = makeCategoryTitles(report, categories);
    const auto functionNames  = makeUniqueFunctionNames(functions);

    // FIXME: workaround for old Clang versions without std::views::values
    const auto values = std::views::transform([](const auto &pair) {
        return pair.second;
    });

    const auto functionsWidth = maximumLength(functionNames);
    const auto resultsWidth  = maximumLength(statuses | values);

    const auto justifyForFunction = [functionsWidth](const QString &text) {
        return text.leftJustified(functionsWidth);
    };

    const auto justifyForCategory = [=](const QString &category, const QString &text) {
        return text.leftJustified(std::max(categoryTitles.at(category).length(), resultsWidth));
    };

    const auto rightJustifyForCategory = [=](const QString &category, const QString &text) {
        return text.rightJustified(std::max(categoryTitles.at(category).length(), resultsWidth));
    };

    const auto fillForCategory = [=](const QString &category, Qt::Alignment alignment) {
        auto text = QString{std::max(categoryTitles.at(category).length(), resultsWidth), u'-'};

        if (alignment & Qt::AlignHCenter)
            text.front() = text.back() = u':';

        return text;
    };

    auto stream = QTextStream{device};

    stream << "# Automated Testing Results" << endl;
    stream << endl;

    stream << "| " << justifyForFunction(u"Functions"_qs);

    for (const auto &cn: categories)
        stream << " | " << justifyForCategory(cn, categoryTitles.at(cn));

    stream << " |" << endl;

    stream << "| " << QString{functionsWidth, u'-'};

    for (const auto &cn: categories)
        stream << " | " << fillForCategory(cn, Qt::AlignCenter);

    stream << " |" << endl;

    for (const auto &fn: functionNames) {
        stream << "| " << justifyForFunction(fn);

        for (const auto &cn: categories) {
            const auto test = results.at({cn, fn});

            stream << " | ";

            if (messageCount(functional::isError)(*test) > 0)
                stream << justifyForCategory(cn, statuses.at(Error));
            else if (messageCount(functional::isFail)(*test) > 0)
                stream << justifyForCategory(cn, statuses.at(Fail));
            else if (messageCount(functional::isSkip)(*test) > 0)
                stream << justifyForCategory(cn, statuses.at(Skip));
            else if (messageCount(functional::isPass)(*test) > 0)
                stream << justifyForCategory(cn, statuses.at(Pass));
            else
                stream << justifyForCategory(cn, {});
        }

        stream << " |" << endl;
    }

    for (const auto &[type, name]: statuses) {
        const auto countMessages = messageCount(functional::hasType(type));

        stream << "| " << justifyForFunction(bold(name));

        for (const auto &cn: categories) {
            const auto messageForType = [cn, countMessages](const Categorized<TestFunction> &f) {
                return category(f) == cn && countMessages(*f.second) > 0;
            };

            const auto count = std::ranges::count_if(results, messageForType);
            stream << " | " << rightJustifyForCategory(cn, QString::number(count));
        }

        stream << " |" << endl;
    }

    return true;
}

bool writeMarkdownTestReport(const TestReport *report, QStringView fileName)
{
    auto file = QFile{fileName.toString()};

    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        qCWarning(lcMarkdownWriter, "%ls: %ls",
                  qUtf16Printable(file.fileName()),
                  qUtf16Printable(file.errorString()));

        return false;
    }

    qCInfo(lcMarkdownWriter,
           R"(Writing test results to "%ls"...)",
           qUtf16Printable(file.fileName()));

    return writeMarkdownTestReport(report, &file);
}

} // namespace reporting
