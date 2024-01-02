#include "githubwriter.h"

#include <QFile>
#include <QLoggingCategory>

namespace reporting {
namespace {

Q_LOGGING_CATEGORY(lcGithubWriter, "reporting.githubwriter")

struct Location
{
    Location(const TestFunction *test) noexcept : test{test} {}
    const TestFunction *test;
};

QTextStream &operator<<(QTextStream &stream, const Location &location)
{
    if (const auto message = std::ranges::find_if(location.test->messages,
                                                  functional::hasLocation);
        message != location.test->messages.cend()) {
        stream << " file=" << message->file
               << ",line=" << message->line;
    }

    return stream;
}

} // namespace

bool writeGithubTestSummary(const TestReport *report, QIODevice *device)
{
    using namespace functional;

    auto stream = QTextStream{device};

    const auto errorCount   = functional::countErrors(report);
    const auto skippedCount = functional::countSkipped(report);
    const auto failureCount = functional::countFailures(report);

    stream << "::notice title=Test Summary::"
           << report->functions.size() << " tests executed, "
           << skippedCount << " tests skipped, "
           << failureCount << " tests failed, "
           << errorCount   << " fatal errors"
           << Qt::endl;

    for (const auto &test : report->functions | std::views::filter(messageCount(isSkip))) {
        stream << "::notice" << Location{&test}
               << ",title=Test skipped::Skipped "
               << test.name << Qt::endl;
    }
 
    for (const auto &test : report->functions | std::views::filter(messageCount(isFail))) {
        stream << "::warning" << Location{&test}
               << ",title=Test failed::Failure in "
               << test.name << Qt::endl;
    }

    for (const auto &test : report->functions | std::views::filter(messageCount(isError))) {
        stream << "::error" << Location{&test}
               << ",title=Fatal error::Fatal error for "
               << test.name << Qt::endl;
    }

    return true;
}

bool writeGithubTestSummary(const TestReport *report, QStringView fileName)
{
    auto file = QFile{fileName.toString()};

    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        qCWarning(lcGithubWriter, "%ls: %ls",
                  qUtf16Printable(file.fileName()),
                  qUtf16Printable(file.errorString()));

        return false;
    }

    qCInfo(lcGithubWriter,
           R"(Writing test summary to "%ls"...)",
           qUtf16Printable(file.fileName()));

    return writeGithubTestSummary(report, &file);
}

} // namespace reporting
