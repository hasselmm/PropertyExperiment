#include "markdownwriter.h"

#include "markup.h"

#include <QFile>
#include <QLoggingCategory>

#include <ranges>
#include <set>

namespace reporting {
namespace {

Q_LOGGING_CATEGORY(lcMarkdownWriter, "reporting.markdownwriter")

using Qt::endl;

using std::ranges::begin;
using std::ranges::end;

// FIXME: workaround for old Clang versions without std::views::values
const auto values = std::views::transform([](const auto &pair) {
    return pair.second;
});

/// Convert the given `range` into a `QList`. This is useful to move implementations
/// out of the header: Ranges make intensive use of templates, and therefore mandate
/// inefficient header-only libraries. Well, or modules (maybe).
///
template<std::ranges::range Range, typename T = std::ranges::range_value_t<Range>>
QList<T> toList(Range &&range)
{
    return {begin(range), end(range)};
}

/// Categorize test functions by splitting up the function names into their namespace
/// or class part, and their function. When merging test reports, we use the prefix
/// to track the origin of a test result. FIXME: We should just add an `origin`
/// field to `TestFunction`.
///
template<typename T>
using Categorized = std::pair<std::pair<QString, QString>, const T *>;

template<typename T>
using CategorizedMap = std::map<std::pair<QString, QString>, const T *>;

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
QList<T> uniqueValues(Iterator first, Iterator last)
{
    auto values = QList<T>{first, last};

    std::ranges::sort(values);
    const auto tail = std::ranges::unique(values);
    values.erase(begin(tail), end(tail));

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

auto makeCategoryTitles(const TestReport *report, const QList<QString> &categories)
{
    const auto makeTitle = [report](QString category) {
        const auto compiler  = propertyText(report, category, u"Compiler"_qs);
        const auto config    = propertyText(report, category, u"CMakeConfig"_qs);
        const auto platform  = propertyText(report, category, u"OperatingSystem"_qs);
        const auto qtVersion = propertyText(report, category, u"QtVersion"_qs, u"Qt %1"_qs);
        const auto title     = QList{qtVersion, platform, compiler, config}.join(u' ');

        return std::make_pair(category, title);
    };

    return categories | std::views::transform(makeTitle);
}

struct BenchmarkLabel
{
    QString id;
    QString name;
    QString dataTag;
};

BenchmarkLabel makeBenchmarkLabel(const QString &functionName)
{
    if (const auto i = functionName.indexOf(u'(');
        i > 0 && functionName.endsWith(u')')) {
        auto dataTag = functionName.mid(i + 1, functionName.length() - i  - 2);
        return {functionName, functionName.left(i - 1), std::move(dataTag)};
    }

    return {functionName, functionName, {}};
}

auto toDataTag(const BenchmarkLabel &label)
{
    return label.dataTag;
}

QString benchmarkResult(const CategorizedMap<TestFunction> &results,
                        const QString                      &categoryName,
                        const QString                      &benchmarkId)
{
    const auto it = results.find({categoryName, benchmarkId});

    if (it == results.cend())
        return QString{};

    return it->second->benchmarks.front().value;
}

/// Write benchmark results as Mermaid diagram embedded in a Markdown code block.
///
void writeBenchmarkChart(QTextStream &stream,
                         const auto  &results,
                         const auto  &categories,
                         const auto  &dataTags,
                         const auto  &benchmarkGroup)
{
    stream << "```mermaid" << endl;
    stream << "xychart-beta" << endl;
    stream << mermaid::Title{benchmarkGroup.front().name} << endl;
    stream << mermaid::XAxis{dataTags} << endl;
    stream << mermaid::YAxis{u"Duration in ms"} << endl;

    // Chart lines
    for (const auto &cn : categories) {
        const auto result = [cn, &results](const auto &label) {
            return benchmarkResult(results, cn, label.id);
        };

        stream << mermaid::Line{benchmarkGroup | std::views::transform(result)} << endl;
    }

    stream << "```" << endl;
    stream << endl;
}

/// Write benchmark results as a Markdown table.
///
void writeBenchmarkTable(QTextStream &stream,
                         const auto  &results,
                         const auto  &categories,
                         const auto  &dataTags,
                         const auto  &benchmarkGroup)
{
    auto header = TableHeader{{u"Build Configuration"_qs}};
    header.updateColumnWidth(0, toList(categories | values));
    header.addColumns(dataTags, Qt::AlignRight);

    stream << header << endl;

    for (const auto &[name, title] : categories) {
        const auto makeResult = [&](int i) {
            auto result = benchmarkResult(results, name, benchmarkGroup[i].id);

            if (const auto dot = result.indexOf(u'.'); dot >= 0) // align by dot
                result = result.leftJustified(dot + 7).replace(u' ', u'\u00A0');

            return header.columns.at(i + 1).alignText(std::move(result));
        };

        auto row = std::vector{header.columns[0].alignText(title)};
        std::ranges::transform(std::views::iota(0, std::ranges::ssize(benchmarkGroup)),
                               std::back_inserter(row), makeResult);
        stream << TableRow{std::move(row)} << endl;
    }

    stream << endl;
}

} // namespace

/// Write a benchmark `report` in Markdown format to `device`.
/// Returns `true` on success.
///
bool writeMarkdownBenchmarkReport(const TestReport *report, QIODevice *device)
{
    // First extract the benchmark reports from `TestReport`.
    //
    auto benchmarks = report->functions
                      | std::views::filter(functional::hasBenchmarks)
                      | std::views::transform(categorize);

    const auto results             = std::map{begin(benchmarks), end(benchmarks)};
    const auto categories          = makeUniqueCategories(benchmarks);
    const auto categoriesWithTitle = makeCategoryTitles(report, categories);
    const auto benchmarkNames      = makeUniqueFunctionNames(benchmarks);
    const auto benchmarkLabels     = benchmarkNames | std::views::transform(makeBenchmarkLabel);

    // Start writing the report...
    //
    auto stream = QTextStream{device};

    stream << Headline1{u"Benchmark Results"} << endl;
    stream << endl;

    for (auto it = begin(benchmarkLabels); it != end(benchmarkLabels); ) {
        const auto currentBenchmarkName = (*it).name;

        const auto isNextBenchmark = [currentBenchmarkName](const auto &label) {
            return label.name != currentBenchmarkName;
        };

        // Group benchmarks with same function name, but different data tag.
        //
        const auto next           = std::find_if(it, end(benchmarkLabels), isNextBenchmark);
        const auto benchmarkGroup = std::ranges::subrange{std::exchange(it, next), next};
        const auto dataTagView    = benchmarkGroup | std::views::transform(toDataTag);
        const auto dataTags       = toList(dataTagView);

        // Generate charts and results table for the currently found benchmark group.
        //
        stream << Headline2{currentBenchmarkName} << endl;
        stream << endl;

        writeBenchmarkChart(stream, results, categories,          dataTags, benchmarkGroup);
        writeBenchmarkTable(stream, results, categoriesWithTitle, dataTags, benchmarkGroup);
    }

    return true;
}

/// Write a test `report` in Markdown format to `device`.
/// Returns `true` on success.
///
bool writeMarkdownTestReport(const TestReport *report, QIODevice *device)
{
    using enum Message::Type;

    const auto statusLabels = std::map<Message::Type, QString> {
        {Error, u"\U0001F4A5 error"_qs},    // C++23: \N{COLLISION SYMBOL}
        {Fail,  u"\U000026A1 failed"_qs},   // C++23: \N{HIGH VOLTAGE SIGN}
        {Skip,  u"\U0001F4A4 skipped"_qs},  // C++23: \N{SLEEPING SYMBOL}
        {Pass,  u"\U00002714 passed"_qs}    // C++23: \N{HEAVY CHECK MARK}
    };

    auto functions = report->functions | std::views::transform(categorize);

    const auto results        = std::map{begin(functions), end(functions)};
    const auto categories     = makeUniqueCategories(functions);
    const auto categoryTitles = makeCategoryTitles(report, categories);
    const auto functionNames  = makeUniqueFunctionNames(functions);

    auto stream = QTextStream{device};

    stream << Headline1{u"Automated Testing Results"} << endl
           << endl;

    auto header = TableHeader{{u"Function"_qs}};
    header.updateColumnWidth(0, functionNames);
    header.addColumns(toList(categoryTitles | values), Qt::AlignCenter);

    stream << header << endl;

    // Report results for each function.
    //
    for (const auto &fn : functionNames) {
        auto row = std::vector{header.columns[0].alignText(fn)};

        for (auto i : std::views::iota(0, std::ranges::ssize(categories))) {
            const auto test = results.at({categories.at(i), fn});
            auto status = QString{};

            if (messageCount(functional::isError)(*test) > 0)
                status = statusLabels.at(Error);
            else if (messageCount(functional::isFail)(*test) > 0)
                status = statusLabels.at(Fail);
            else if (messageCount(functional::isSkip)(*test) > 0)
                status = statusLabels.at(Skip);
            else if (messageCount(functional::isPass)(*test) > 0)
                status = statusLabels.at(Pass);

            row.emplace_back(header.columns[i + 1].alignText(status));
        }

        stream << TableRow{std::move(row)} << endl;
    }

    // Produce the summary rows.
    //
    for (const auto &[type, name]: statusLabels) {
        const auto countMessages = messageCount(functional::hasType(type));
        auto row = std::vector{header.columns[0].alignText(name)};

        for (auto i : std::views::iota(0, std::ranges::ssize(categories))) {
            const auto &cn = categories.at(i);
            const auto messageForType = [cn, countMessages](const Categorized<TestFunction> &f) {
                return category(f) == cn && countMessages(*f.second) > 0;
            };

            const auto count = std::ranges::count_if(results, messageForType);
            row.emplace_back(header.columns[i + 1].alignText(QString::number(count)));
        }

        stream << TableRow{std::move(row)} << endl;
    }

    return true;
}

/// Write a benchmark `report` in Markdown format to `fileName`.
/// Returns `true` on success.
///
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

/// Write a test `report` in Markdown format to `fileName`.
/// Returns `true` on success.
///
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
