#ifndef REPORTING_MARKUP_H
#define REPORTING_MARKUP_H

#include <QList>
#include <QString>
#include <QTextStream>

#include <ranges>

namespace reporting {
namespace markup::detail {

template<typename T, T> inline constexpr auto prefix = QStringView{};
template<typename T, T> inline constexpr auto joiner = QStringView{};
template<typename T, T> inline constexpr auto suffix = QStringView{};

template<auto T>
struct Fragment
{
    using FragmentType = decltype(T);

    Fragment(QString &&text) noexcept
        : text{std::move(text)}
    {}

    Fragment(const QString &text) noexcept
        : text{std::move(text)}
    {}

    template<std::size_t N>
    Fragment(const char16_t (& text)[N])
        : text{QString::fromUtf16(text, N - 1)}
    {}

    friend QTextStream &operator<<(QTextStream &stream, const Fragment<T> &fragment)
    {
        return stream << prefix<FragmentType, T> << fragment.text << suffix<FragmentType, T>;
    }

    operator QString() const noexcept
    {
        return prefix<FragmentType, T> % text % suffix<FragmentType, T>;
    }

    QString text;
};

template<auto T, std::ranges::range Range>
struct Sequence
{
    using SequenceType = decltype(T);
    using IteratorType = std::ranges::iterator_t<Range>;
    using SentinelType = std::ranges::sentinel_t<Range>;

    Sequence(IteratorType first, SentinelType last) noexcept
        : first{std::move(first)}
        , last{std::move(last)}
    {}

    Sequence(Range &&range) noexcept
        : first{std::ranges::begin(range)}
        , last{std::ranges::end(range)}
    {}

    friend QTextStream &operator<<(QTextStream &stream, const Sequence<T, Range> &sequence)
    {
        stream << prefix<SequenceType, T>;

        if (auto it = sequence.first; it != sequence.last) {
            stream << *it;

            while (++it != sequence.last)
                stream << joiner<SequenceType, T> << *it;
        }

        return stream << suffix<SequenceType, T>;
    }

    IteratorType first;
    SentinelType last;
};

} // namespace markup::detail

#define REPORTING_MARKUP_DECLARE_FRAGMENT(FRAGMENT)                             \
using FRAGMENT = markup::detail::Fragment<detail::FRAGMENT>;

#define REPORTING_MARKUP_DECLARE_SEQUENCE(SEQUENCE)                             \
template<std::ranges::range Range>                                              \
struct SEQUENCE : public markup::detail::Sequence<detail::SEQUENCE, Range>      \
{ using markup::detail::Sequence<detail::SEQUENCE, Range>::Sequence; };         \
                                                                                \
template<std::ranges::range Range>                                              \
SEQUENCE(Range &&) -> SEQUENCE<Range>;

// template<std::forward_iterator Iterator>
//     SEQUENCE(Iterator, Iterator) -> SEQUENCE<Iterator>;

inline namespace markdown {
namespace detail {

enum FragmentType
{
    Headline1,
    Headline2,
    Bold,
};

enum SequenceType
{
    TableRow,
};

} // namespace detail

REPORTING_MARKUP_DECLARE_FRAGMENT(Headline1)
REPORTING_MARKUP_DECLARE_FRAGMENT(Headline2)
REPORTING_MARKUP_DECLARE_FRAGMENT(Bold)
REPORTING_MARKUP_DECLARE_SEQUENCE(TableRow)

} // namespace markdown

namespace markup::detail {

using enum markdown::detail::FragmentType;
using enum markdown::detail::SequenceType;

using MarkdownFragmentType = markdown::detail::FragmentType;
using MarkdownSequenceType = markdown::detail::SequenceType;

template<> inline constexpr auto prefix<MarkdownFragmentType, Headline1> = QStringView{u"# "};
template<> inline constexpr auto prefix<MarkdownFragmentType, Headline2> = QStringView{u"## "};
template<> inline constexpr auto prefix<MarkdownFragmentType, Bold>      = QStringView{u"**"};
template<> inline constexpr auto suffix<MarkdownFragmentType, Bold>      = QStringView{u"**"};
template<> inline constexpr auto prefix<MarkdownSequenceType, TableRow>  = QStringView{u"| "};
template<> inline constexpr auto joiner<MarkdownSequenceType, TableRow>  = QStringView{u" | "};
template<> inline constexpr auto suffix<MarkdownSequenceType, TableRow>  = QStringView{u" |"};

} // namespace markup::detail

inline namespace mermaid {
namespace detail {

enum FragmentType
{
    Title,
    YAxis
};

enum SequenceType
{
    XAxis,
    Line,
};

} // namespace detail

REPORTING_MARKUP_DECLARE_FRAGMENT(Title)
REPORTING_MARKUP_DECLARE_SEQUENCE(XAxis)
REPORTING_MARKUP_DECLARE_FRAGMENT(YAxis)
REPORTING_MARKUP_DECLARE_SEQUENCE(Line)

} // namespace mermaid

namespace markup::detail {

using enum mermaid::detail::FragmentType;
using enum mermaid::detail::SequenceType;

using MermaidFragmentType = mermaid::detail::FragmentType;
using MermaidSequenceType = mermaid::detail::SequenceType;

template<> inline constexpr auto prefix<MermaidFragmentType, Title> = QStringView{u"  title \""};
template<> inline constexpr auto suffix<MermaidFragmentType, Title> = QStringView{u"\""};
template<> inline constexpr auto prefix<MermaidSequenceType, XAxis> = QStringView{u"  x-axis [\""};
template<> inline constexpr auto joiner<MermaidSequenceType, XAxis> = QStringView{u"\", \""};
template<> inline constexpr auto suffix<MermaidSequenceType, XAxis> = QStringView{u"\"]"};
template<> inline constexpr auto prefix<MermaidFragmentType, YAxis> = QStringView{u"  y-axis \""};
template<> inline constexpr auto suffix<MermaidFragmentType, YAxis> = QStringView{u"\""};
template<> inline constexpr auto prefix<MermaidSequenceType, Line>  = QStringView{u"  line ["};
template<> inline constexpr auto joiner<MermaidSequenceType, Line>  = QStringView{u", "};
template<> inline constexpr auto suffix<MermaidSequenceType, Line>  = QStringView{u"]"};

} // namespace markup::detail


inline namespace markdown {

struct TableColumn
{
    TableColumn(QString       title = {},
                Qt::Alignment align = Qt::AlignLeft) noexcept
        : width{std::max(title.length(), qsizetype{5})}
        , title{std::move(title)}
        , align{align}
    {}

    QString alignText(const QString &text) const;

    qsizetype       width;
    QString         title;
    Qt::Alignment   align;
};

struct TableHeader
{
    TableHeader(QList<TableColumn> columns = {}) noexcept
        : columns{std::move(columns)}
    {}

    void addColumns(QStringList titles, Qt::Alignment align = Qt::AlignLeft);
    void updateColumnWidth(int row, QStringList values);

    [[nodiscard]] static QString makeTitle(const TableColumn &column) noexcept;
    [[nodiscard]] static QString makeUnderline(const TableColumn &column) noexcept;

    friend QTextStream &operator<<(QTextStream &stream, const TableHeader &header)
    {
        return stream << TableRow{header.columns | std::views::transform(makeTitle)} << Qt::endl
                      << TableRow{header.columns | std::views::transform(makeUnderline)};
    }

    QList<TableColumn> columns;
};

} // namespace markdown
} // namespace reporting

#endif // REPORTING_MARKUP_H
