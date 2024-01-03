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

QString propertyText(const TestReport *report, const QString &category,
                     const QString &key, const QString &formatString = {})
{
    auto value = report->properties.at(category + u"::"_qs + key);

    if (!formatString.isEmpty())
        value = formatString.arg(value);

    return value.replace(u' ', u'\u00a0');
}

auto makeCategoryTitles(const TestReport *report, const std::vector<QString> &categories)
{
    const auto makeTitle = [report](QString category) {
        const auto compiler  = propertyText(report, category, u"Compiler"_qs);
        const auto config    = propertyText(report, category, u"CMakeConfig"_qs);
        const auto platform  = propertyText(report, category, u"OperatingSystem"_qs);
        const auto qtVersion = propertyText(report, category, u"QtVersion"_qs, u"Qt %1"_qs);

        const auto title = QList{qtVersion, platform, compiler, config};
        return std::make_pair(std::move(category), title.join(u' '));
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

    const auto dataTag = [](const auto &p) {
        return std::get<DataTag>(p);
    };

    for (auto it = taggedBenchmarkNames.begin(); it != taggedBenchmarkNames.end(); ) {
        const auto benchmarkName = std::get<BenchmarkName>(*it);
        const auto isNextBenchmark = [benchmarkName](const auto &tuple) {
            return std::get<BenchmarkName>(tuple) != benchmarkName;
        };

        const auto next           = std::find_if(it, taggedBenchmarkNames.end(), isNextBenchmark);
        const auto benchmarkGroup = std::ranges::subrange{std::exchange(it, next), next};
        const auto dataTagView    = benchmarkGroup | std::views::transform(dataTag);

        stream << "## " << benchmarkName << endl;
        stream << endl;

        // Render Mermaid line chart with benchmark results
        //
        stream << "```mermaid" << endl;
        stream << "xychart-beta" << endl;
        stream << "  title \"" << benchmarkName << '"' << endl;
        stream << "  x-axis [";

        if (auto dataTag = dataTagView.begin(); dataTag != dataTagView.end()) {
            stream << '"' << *dataTag << '"';

            while (++dataTag != dataTagView.end())
                stream << ", \"" << *dataTag << '"';
        }

        stream << "]" << endl;
        stream << "  y-axis \"Duration in ms\"" << endl;

        // Chart lines
        for (const auto &cn : categories) {
            stream << "  line [";

            if (auto it = benchmarkGroup.begin(); it != benchmarkGroup.end()) {
                stream << measurement(cn, std::get<BenchmarkId>(*it));

                while (++it != benchmarkGroup.end())
                    stream << ", " << measurement(cn, std::get<BenchmarkId>(*it));
            }

            stream << "]" << endl;
        }

        stream << "```" << endl;
        stream << endl;

        // Render Markdown table with benchmark results
        //
        const auto categoriesTitle = u"Build Configuration"_qs;
        const auto categoriesWidth = std::max(categoriesTitle.length(),
                                              maximumLength(categories));

        // Table header
        stream << "| " << categoriesTitle.leftJustified(categoriesWidth) << " |";

        for (const auto &dataTag : dataTagView)
            stream << ' ' << dataTag << " |";

        stream << endl;
        stream << "| " << QString{categoriesWidth, u'-'} << " |";

        for (const auto &dataTag : dataTagView)
            stream << ' ' << QString{dataTag.length() - 1, u'-'} << ": |";

        stream << endl;

        // Table body
        for (const auto &cn : categories) {
            stream << "| " << cn.leftJustified(categoriesWidth) << " |";

            for (const auto &run : benchmarkGroup) {
                const auto columnWidth = dataTag(run).length();
                const auto duration = measurement(cn, std::get<BenchmarkId>(run));
                stream << ' ' << duration.leftJustified(columnWidth) << " |";
            }

            stream << endl;
        }

        stream << endl;
    }


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
        {Error, u":fire:    error"_qs},
        {Fail,  u":zap:    failed"_qs},
        {Skip,  u":zzz:   skipped"_qs},
        {Pass,  u"&#x2714; passed"_qs} // ":heavy_check_mark:" is pretty long!
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

    // FIXME: merge table (header) rendering with writeMarkdownBenchmarkReport()
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
