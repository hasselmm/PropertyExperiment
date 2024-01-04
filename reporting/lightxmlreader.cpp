#include "lightxmlreader.h"

#include <QFile>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QXmlStreamReader>

namespace reporting {
namespace {

Q_LOGGING_CATEGORY(lcLightXmlReader, "reporting.lightxmlreader")

auto coalesce(auto &&text)
{
    return text;
}

template<typename T, typename... Tail>
auto coalesce(T &&text, Tail &&...tail)
{
    if (text.isEmpty())
        return coalesce(std::forward<Tail>(tail)...);

    return text;
}

template<typename T>
auto byTag(const std::vector<T> &items)
{
    auto result = std::map<QString, std::vector<T>>{};

    for (const auto &i: items)
        result[i.tag].emplace_back(i);

    return result;
}

template<typename T>
auto byTagOccurance(const std::vector<T> &items)
{
    auto result = std::vector<QString>{};

    for (const auto &i: items) {
        if (std::ranges::find(result, i.tag) == result.end())
            result.emplace_back(i.tag);
    }

    return result;
}

std::vector<TestFunction> flatten(const std::vector<TestFunction> &input)
{
    auto result = std::vector<TestFunction>{};

    for (const auto &function: input) {
        Q_ASSERT(!function.name.isEmpty());

        auto messagesByTag   = byTag(function.messages);
        auto benchmarksByTag = byTag(function.benchmarks);
        auto tagsByOccurance = byTagOccurance(function.messages);

        for (const auto &tag: tagsByOccurance) {
            result.emplace_back(TestFunction{});

            auto name = function.name;

            if (!tag.isEmpty()) {
                name += '(';
                name += tag;
                name += ')';
            }

            result.back().name = std::move(name);

            if (const auto it = messagesByTag.find(tag); it != messagesByTag.end()) {
                auto messages = it->second;

                result.back().messages.reserve(messages.size());

                const auto itFatal = std::ranges::find_if(messages, functional::hasType(u"qfatal"));
                const auto itFail  = std::ranges::find_if(messages, functional::hasType(u"fail"));

                if (itFatal != messages.cend() && itFail != messages.cend()) {
                    auto text = itFatal->text + '\n' + itFail->text;
                    auto file = itFatal->file;
                    auto line = itFatal->line;

                    if (file.isEmpty()) {
                        file = itFail->file;
                        line = itFail->line;
                    }

                    result.back().messages.emplace_back(std::move(itFatal->type),
                                                        std::move(file),
                                                        std::move(line),
                                                        std::move(text),
                                                        std::move(itFatal->tag));

                    const auto first = std::min(itFail, itFatal);
                    const auto last  = std::max(itFail, itFatal);

                    messages.erase(last);
                    messages.erase(first);
                }

                std::ranges::copy(messages, std::back_inserter(result.back().messages));
            }

            if (const auto it = benchmarksByTag.find(tag); it != benchmarksByTag.end()) {
                const auto &benchmarks = it->second;
                result.back().messages.reserve(benchmarks.size());
                std::ranges::copy(benchmarks, std::back_inserter(result.back().benchmarks));
            }
        }
    }

    return result;
}

std::unique_ptr<TestReport> readLightXml(QIODevice *device, QString fileName)
{
    if (!fileName.isEmpty()) {
        qCInfo(lcLightXmlReader,
               R"(Reading test report from "%ls"...)",
               qUtf16Printable(fileName));
    }

    auto report = std::make_unique<TestReport>();
    report->timestamp = QDateTime::currentDateTime();

    auto messages = std::vector<Message>();

    auto xml = QXmlStreamReader{};
    xml.addData("<lightxml>");
    xml.addData(device->readAll());
    xml.addData("</lightxml>");

    if (!xml.readNextStartElement() || xml.name() != u"lightxml"_qs)
        xml.raiseError("Unexpected element");

    while (xml.readNextStartElement()) {
        if (xml.name() == u"Environment"_qs) {
            while (xml.readNextStartElement()) {
                report->properties.emplace(xml.name().toString(),
                                          xml.readElementText());
            }
        } else if (xml.name() == u"Duration"_qs) {
            report->duration = xml.readElementText();
        } else if (xml.name() == u"TestFunction"_qs) {
            report->functions.emplace_back(TestFunction{});
            auto &function = report->functions.back();

            const auto attrs = xml.attributes();
            function.name    = attrs.value("name").toString();

            while (xml.readNextStartElement()) {
                if (xml.name() == u"Incident"_qs
                    || xml.name() == u"Message"_qs) {
                    function.messages.emplace_back(Message{});
                    auto &message = function.messages.back();

                    const auto attrs = xml.attributes();
                    message.type     = attrs.value("type").toString();
                    message.line     = attrs.value("line").toString();
                    message.file     = attrs.value("file").toString();

                    while (xml.readNextStartElement()) {
                        if (xml.name() == u"DataTag"_qs && message.tag.isEmpty()) {
                            message.tag = xml.readElementText();
                        } else if (xml.name() == u"Description"_qs && message.text.isEmpty()) {
                            message.text = xml.readElementText();
                        } else {
                            xml.raiseError(u"Unexpected element: "_qs + xml.name().toString());
                            break;
                        }
                    }
                } else if (xml.name() == u"BenchmarkResult"_qs) {
                    function.benchmarks.emplace_back(Benchmark{});
                    auto &benchmark = function.benchmarks.back();

                    const auto attrs     = xml.attributes();
                    benchmark.metric     = attrs.value("metric").toString();
                    benchmark.tag        = attrs.value("tag").toString();
                    benchmark.value      = attrs.value("value").toString();
                    benchmark.iterations = attrs.value("iterations").toString();

                    xml.skipCurrentElement();
                } else if (xml.name() == u"Duration"_qs) {
                    function.duration = xml.readElementText();
                } else {
                    xml.raiseError(u"Unexpected element: "_qs + xml.name().toString());
                    break;
                }
            }
        } else {
            xml.raiseError(u"Unexpected element: "_qs + xml.name().toString());
            break;
        }
    }

    if (xml.hasError()) {
        qCWarning(lcLightXmlReader, "%ls%sline %lld: %ls",
                  qUtf16Printable(fileName),
                  fileName.isEmpty() ? "" : ", ",
                  xml.lineNumber(),
                  qUtf16Printable(xml.errorString()));

        return {};
    }

    report->functions = flatten(report->functions);

    return report;
}

} // namespace

std::unique_ptr<TestReport> readLightXml(QIODevice *device)
{
    return readLightXml(device, {});
}

std::unique_ptr<TestReport> readLightXml(QFile *file)
{
    return readLightXml(file, file->fileName());
}

std::unique_ptr<TestReport> readLightXml(QStringView fileName)
{
    const auto fileInfo = QFileInfo{fileName.toString()};
    auto file = QFile{fileInfo.fileName()};

    if (!file.open(QFile::ReadOnly)) {
        qCWarning(lcLightXmlReader, "%ls: %ls",
                 qUtf16Printable(file.fileName()),
                 qUtf16Printable(file.errorString()));

        return {};
    }

    auto report = readLightXml(&file);

    if (report) {
        report->name = fileInfo.completeBaseName();
        report->timestamp = fileInfo.lastModified();
    }

    return report;
}

} // namespace reporting
