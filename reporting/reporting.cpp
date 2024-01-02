#include "reporting.h"

#include "githubwriter.h"
#include "junitwriter.h"
#include "lightxmlreader.h"
#include "markdownwriter.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QHostInfo>
#include <QLoggingCategory>

#include <ranges>

namespace reporting {

Q_LOGGING_CATEGORY(lcReporting, "reporting")

using TestReportPtr = std::unique_ptr<TestReport>;

TestReportPtr merge(const std::vector<TestReportPtr> &reports)
{
    auto result = std::make_unique<TestReport>();

    for (const auto &report : reports) {
        const auto prefix = report->name + u"::"_qs;

        const auto prefixedFunction = [prefix](TestFunction function) {
            function.name = prefix + function.name;
            return function;
        };

        const auto prefixedProperty = [prefix](const auto &pair) {
            return std::make_pair(prefix + pair.first, pair.second);
        };

        std::ranges::transform(report->functions,
                               std::back_inserter(result->functions),
                               prefixedFunction);

        std::ranges::transform(report->properties,
                               std::inserter(result->properties,
                                             result->properties.end()),
                               prefixedProperty);
    }

    return result;
}

class ParseReports : public QCoreApplication
{
public:
    using QCoreApplication::QCoreApplication;

    int run();

private:
    TestReportPtr mergeReports(QStringList fileNameList);

    bool writeAutoTestReport (const TestReport *report, QStringList fileNameList);
    bool writeAutoTestSummary(const TestReport *report, QStringList fileNameList);
    bool writeBenchmarkReport(const TestReport *report, QStringList fileNameList);

    QString m_hostname;
};

TestReportPtr ParseReports::mergeReports(QStringList fileNameList)
{
    auto reports = std::vector<TestReportPtr>{};

    if (fileNameList.isEmpty()) {
        qCWarning(lcReporting, "No input filenames with test reports");
        return {};
    }

    for (const auto &fileName: fileNameList) {
        auto report = readLightXml(fileName);

        if (!report)
            return {};

        report->hostname = m_hostname;
        reports.emplace_back(std::move(report));
    }

    return merge(reports);
}

bool ParseReports::writeAutoTestReport(const TestReport *report, QStringList fileNameList)
{
    for (const auto &fileName: fileNameList) {
        if (fileName.endsWith(u"-junit.xml") || fileName.endsWith(u".junit")) {
            if (!writeJUnitReport(report, std::move(fileName)))
                return false;
        } else if (fileName.endsWith(u".md")) {
            if (!writeMarkdownTestReport(report, std::move(fileName)))
                return false;
        } else {
            qCWarning(lcReporting,
                      "Unsupported filename for automated testing report: %ls",
                      qUtf16Printable(fileName));

            return false;
        }
    }

    return true;
}

bool ParseReports::writeAutoTestSummary(const TestReport *report, QStringList fileNameList)
{
    for (const auto &fileName: fileNameList) {
        if (fileName.endsWith(u".github")) {
            if (!writeGithubTestSummary(report, std::move(fileName)))
                return false;
        } else {
            qCWarning(lcReporting,
                      "Unsupported filename for automated testing summary: %ls",
                      qUtf16Printable(fileName));

            return false;
        }
    }

    return true;
}

bool ParseReports::writeBenchmarkReport(const TestReport *report, QStringList fileNameList)
{
    for (const auto &fileName: fileNameList) {
        if (fileName.endsWith(u".md")) {
            if (!writeMarkdownBenchmarkReport(report, std::move(fileName)))
                return false;
        } else {
            qCWarning(lcReporting,
                      "Unsupported filename for benchmark report: %ls",
                      qUtf16Printable(fileName));

            return false;
        }
    }

    return true;
}

int ParseReports::run()
{
    const auto argumentNameAutoTestReport   = u"autotest-report"_qs;
    const auto argumentNameAutoTestSummary  = u"autotest-summary"_qs;
    const auto argumentNameBenchmarkReport  = u"benchmark-report"_qs;
    const auto argumentNameHostname         = u"hostname"_qs;

    auto commandLine = QCommandLineParser{};

    commandLine.addPositionalArgument(tr("INPUT-FILENAME[...]"),
                                      tr("The filenames of the test reports to read"
                                         " (in Qt Test's light XML format)"));

    commandLine.addOption({argumentNameHostname,
                           tr("The name of the computer on which the test was run"),
                           tr("HOSTNAME")});
    commandLine.addOption({argumentNameAutoTestReport,
                           tr("The filename of the automated testing report to write, "
                              "supported formats: JUnit, Markdown"),
                           tr("FILENAME")});
    commandLine.addOption({argumentNameAutoTestSummary,
                           tr("The filename of the automated testing summary to write, "
                              "supported formats: Github"),
                           tr("FILENAME")});
    commandLine.addOption({argumentNameBenchmarkReport,
                           tr("The filename of the benchmark report to write, "
                              "supported formats: Markdown"),
                           tr("FILENAME")});

    commandLine.setApplicationDescription(tr("Processes Qt Test reports in light XML to produce "
                                             "various summaries and reports. The actual format "
                                             "of a summary or report is determinated by the "
                                             "given file extension."));

    commandLine.addVersionOption();
    commandLine.addHelpOption();

    commandLine.process(arguments());

    m_hostname = commandLine.isSet(argumentNameHostname)
               ? commandLine.value(argumentNameHostname)
               : QHostInfo{}.hostName();

    const auto mergedReport = mergeReports(commandLine.positionalArguments());

    if (!mergedReport)
        return EXIT_FAILURE;

    if (!writeAutoTestReport(mergedReport.get(), commandLine.values(argumentNameAutoTestReport)))
        return EXIT_FAILURE;
    if (!writeAutoTestSummary(mergedReport.get(), commandLine.values(argumentNameAutoTestSummary)))
        return EXIT_FAILURE;
    if (!writeBenchmarkReport(mergedReport.get(), commandLine.values(argumentNameBenchmarkReport)))
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}

} // namespace reporting

int main(int argc, char *argv[])
{
    return reporting::ParseReports{argc, argv}.run();
}
