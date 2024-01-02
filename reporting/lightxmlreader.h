#ifndef REPORTING_LIGHTXMLREADER_H
#define REPORTING_LIGHTXMLREADER_H

#include "reporting.h"

class QIODevice;
class QFile;

namespace reporting {

std::unique_ptr<TestReport> readLightXml(QIODevice    *device);
std::unique_ptr<TestReport> readLightXml(QFile          *file);
std::unique_ptr<TestReport> readLightXml(QStringView fileName);

} // namespace reporting

#endif // REPORTING_LIGHTXMLREADER_H
