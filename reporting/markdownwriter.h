#ifndef REPORTING_MARKDOWNWRITER_H
#define REPORTING_MARKDOWNWRITER_H

#include "reporting.h"

namespace reporting {

bool writeMarkdownBenchmarkReport(const TestReport *report, QIODevice    *device);
bool writeMarkdownBenchmarkReport(const TestReport *report, QStringView fileName);

bool writeMarkdownTestReport     (const TestReport *report, QIODevice    *device);
bool writeMarkdownTestReport     (const TestReport *report, QStringView fileName);

} // namespace reporting

#endif // REPORTING_MARKDOWNWRITER_H
