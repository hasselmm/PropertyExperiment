#ifndef REPORTING_GITHUBWRITER_H
#define REPORTING_GITHUBWRITER_H

#include "reporting.h"

namespace reporting {

bool writeGithubTestSummary(const TestReport *report, QIODevice    *device);
bool writeGithubTestSummary(const TestReport *report, QStringView fileName);

} // namespace reporting

#endif // REPORTING_GITHUBWRITER_H
