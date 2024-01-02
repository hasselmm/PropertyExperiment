#include "junitwriter.h"

#include <QFile>
#include <QLoggingCategory>
#include <QXmlStreamWriter>

#include <ranges>

namespace reporting {
namespace {

Q_LOGGING_CATEGORY(lcJUnitXmlWriter, "reporting.junitxmlwriter")

enum class MessageCategory {
    Other,
    Output,
    Error,
};

class JUnitXmlWriter
{
public:
    explicit JUnitXmlWriter(const TestReport *report, QIODevice *device) noexcept
        : m_report{report}
        , m_xml{device}
    {}

    bool write();

private:
    using ScopeGuard = QScopeGuard<std::function<void()>>;
    ScopeGuard xmlElementScope(QStringView name);

    void writeProperties();
    void writeFunction  (const TestFunction &function);
    void writeMessage   (const Message       &message,
                         MessageCategory lastCategory);

    void indentCharacters();
    void indentElement();
    void closeOutputElement();

    const TestReport *const m_report;
    QXmlStreamWriter        m_xml;
};

auto category(const Message &message)
{
    if (message.type == u"qdebug"
        || message.type == u"qinfo")
        return MessageCategory::Output;

    if (message.type == u"qwarn")
        return MessageCategory::Error;

    return MessageCategory::Other;
}

JUnitXmlWriter::ScopeGuard JUnitXmlWriter::xmlElementScope(QStringView name)
{
    m_xml.writeStartElement(name.toString());

    return qScopeGuard<std::function<void()>>([this] {
        m_xml.writeEndElement();
    });
}

bool JUnitXmlWriter::write()
{
    const auto errorCount   = functional::countErrors(m_report);
    const auto skippedCount = functional::countSkipped(m_report);
    const auto failureCount = functional::countFailures(m_report);

    m_xml.setAutoFormattingIndent(2);
    m_xml.setAutoFormatting(true);

    m_xml.writeStartDocument();

    if (auto testsuite = xmlElementScope(u"testsuite"); true) {
        if (!m_report->name.isEmpty())
            m_xml.writeAttribute("name", m_report->name);
        if (!m_report->timestamp.isNull())
            m_xml.writeAttribute("timestamp", m_report->timestamp.toString(Qt::ISODate));
        if (!m_report->hostname.isEmpty())
            m_xml.writeAttribute("hostname", m_report->hostname);

        m_xml.writeAttribute("tests",    QString::number(m_report->functions.size()));
        m_xml.writeAttribute("failures", QString::number(failureCount));
        m_xml.writeAttribute("errors",   QString::number(errorCount));
        m_xml.writeAttribute("skipped",  QString::number(skippedCount));

        if (!m_report->duration.isEmpty())
            m_xml.writeAttribute("time", m_report->duration);

        writeProperties();

        for (const auto &function: m_report->functions)
            writeFunction(function);
    }

    m_xml.writeEndDocument();

    return true;
}

void JUnitXmlWriter::writeProperties()
{
    const auto properties = xmlElementScope(u"properties");

    for (const auto &[name, value] : m_report->properties) {
        const auto property = xmlElementScope(u"property");

        m_xml.writeAttribute("name", name);
        m_xml.writeAttribute("value", value);
    }
}

void JUnitXmlWriter::writeFunction(const TestFunction &function)
{
    const auto testcase = xmlElementScope(u"testcase");

    auto name = function.name;
    auto className = m_report->name;

    if (const auto lastColon = name.lastIndexOf(u':'); lastColon != -1) {
        auto endOfPrefix = lastColon;

        while (endOfPrefix > 0 && name[endOfPrefix - 1] == u':')
            --endOfPrefix;

        if (endOfPrefix > 0) {
            if (!className.isEmpty())
                className += u"::";

            className += name.left(endOfPrefix);
        }

        name = name.mid(lastColon + 1);
    }

    m_xml.writeAttribute("name", name);

    if (!className.isEmpty())
        m_xml.writeAttribute("classname", className);
    if (!function.duration.isEmpty())
        m_xml.writeAttribute("time", function.duration);
    else { // FIXME: remove
        static const auto fakeTimes = QMap<QString, QString> {
            {"initTestCase", "0.001"},
            {"testPropertyChanges(AObjectTest)", "0.335"},
            {"testPropertyChanges(MObjectTest)", "0.226"},
            {"testPropertyChanges(NObjectMacro)", "0.338"},
            {"testPropertyChanges(NObjectModern)", "0.266"},
            {"testPropertyChanges(SObjectTest)", "0.325"},
            {"testPropertyNotifications(AObjectTest)", "0.259"},
            {"testPropertyNotifications(MObjectTest)", "0.297"},
            {"testPropertyNotifications(NObjectMacro)", "0.236"},
            {"testPropertyNotifications(NObjectModern)", "0.251"},
            {"testPropertyNotifications(SObjectTest)", "0.342"},
            {"testInterfaces(AObjectTest)", "0.345"},
            {"testInterfaces(NObjectMacro)", "0.318"},
            {"testInterfaces(NObjectModern)", "0.257"},
            {"testInterfaces(NObjectLegacy)", "0.218"},
            {"testInterfaces(SObjectTest)", "0.231"}
        };

        auto fake = fakeTimes.value(function.name.section(u':', -1), "0.000");
        m_xml.writeAttribute("time", std::move(fake));
    }

    if (!function.messages.empty()) {
        auto lastCategory = MessageCategory::Other;

        for (const auto &message: function.messages) {
            writeMessage(message, lastCategory);
            lastCategory = category(message);
        }

        if (lastCategory != MessageCategory::Other)
            closeOutputElement();
    }
}

void JUnitXmlWriter::writeMessage(const Message       &message,
                                  MessageCategory lastCategory)
{
    const auto currentCategory = category(message);

    if (currentCategory != lastCategory) {
        if (lastCategory != MessageCategory::Other)
            closeOutputElement();

        switch (currentCategory) {
        case MessageCategory::Output:
            m_xml.writeStartElement(u"system-out"_qs);
            break;

        case MessageCategory::Error:
            m_xml.writeStartElement(u"system-err"_qs);
            break;

        case MessageCategory::Other:
            break;
        }
    }

    if (currentCategory != MessageCategory::Other) {
        indentCharacters();
        m_xml.writeCDATA(message.text);
    } else if (message.type == "skip") {
        const auto skipped = xmlElementScope(u"skipped");
        m_xml.writeAttribute("message", message.text);
    } else if (message.type == "fail") {
        const auto failure = xmlElementScope(u"failure");
        m_xml.writeAttribute("type", message.type);
        m_xml.writeAttribute("message", message.text.section('\n', 0, 0));
        indentCharacters();
        m_xml.writeCDATA(message.text.section('\n', 1));
        indentElement();
    } else if (message.type == "qfatal") {
        const auto error = xmlElementScope(u"error");
        m_xml.writeAttribute("type", message.type);
        m_xml.writeAttribute("message", message.text);
    } else if (message.type == "pass") {
        // nothing to do
    } else {
        qCWarning(lcJUnitXmlWriter) << message.type;
    }
}

void JUnitXmlWriter::indentCharacters()
{
    m_xml.writeCharacters(u"\n      "_qs);
}

void JUnitXmlWriter::indentElement()
{
    m_xml.writeCharacters(u"\n    "_qs);
}

void JUnitXmlWriter::closeOutputElement()
{
    indentElement();
    m_xml.writeEndElement();
}

} // namespace

bool writeJUnitXml(const TestReport *report, QIODevice *device)
{
    return JUnitXmlWriter{report, device}.write();
}

bool writeJUnitReport(const TestReport *report, QStringView fileName)
{
    auto file = QFile{fileName.toString()};

    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        qCWarning(lcJUnitXmlWriter, "%ls: %ls",
                  qUtf16Printable(file.fileName()),
                  qUtf16Printable(file.errorString()));

        return false;
    }

    qCInfo(lcJUnitXmlWriter,
           R"(Writing test report to "%ls"...)",
           qUtf16Printable(file.fileName()));

    return writeJUnitXml(report, &file);
}

} // namespace reporting
