#ifndef REPORTING_JUNITWRITER_H
#define REPORTING_JUNITWRITER_H

#include "reporting.h"

namespace reporting {

bool writeJUnitXml(const TestReport *report, QIODevice    *device);
bool writeJUnitReport(const TestReport *report, QStringView fileName);

} // namespace reporting

#endif // REPORTING_JUNITRITER_H
