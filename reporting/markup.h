#ifndef REPORTING_MARKUP_H
#define REPORTING_MARKUP_H

#include <QList>
#include <QString>
#include <QTextStream>

#include <ranges>

namespace reporting {
namespace markup::detail {

template<typename T, T> constexpr auto prefix = QStringView{};
template<typename T, T> constexpr auto joiner = QStringView{};
template<typename T, T> constexpr auto suffix = QStringView{};

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
        : text{QString::fromUtf16(text, N)}
    {}

    friend QTextStream &operator<<(QTextStream &stream, const Fragment<T> &fragment)
    {
        return stream << prefix<FragmentType, T> << fragment.text << suffix<FragmentType, T>;
    }

    operator QString() const noexcept
    {
        static const auto s_prefix = prefix<FragmentType, T>.toString();
        static const auto s_suffix = suffix<FragmentType, T>.toString();

        return s_prefix + text + s_suffix;
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
        : first{std::begin(range)}
        , last{std::end(range)}
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
template<> constexpr auto prefix<markdown::detail::FragmentType, markdown::detail::Headline1> = QStringView{u"# "};
template<> constexpr auto prefix<markdown::detail::FragmentType, markdown::detail::Headline2> = QStringView{u"## "};
template<> constexpr auto prefix<markdown::detail::FragmentType, markdown::detail::Bold>      = QStringView{u"**"};
template<> constexpr auto suffix<markdown::detail::FragmentType, markdown::detail::Bold>      = QStringView{u"**"};
template<> constexpr auto prefix<markdown::detail::SequenceType, markdown::detail::TableRow>  = QStringView{u"| "};
template<> constexpr auto joiner<markdown::detail::SequenceType, markdown::detail::TableRow>  = QStringView{u" | "};
template<> constexpr auto suffix<markdown::detail::SequenceType, markdown::detail::TableRow>  = QStringView{u" |"};
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
template<> constexpr auto prefix<mermaid::detail::FragmentType, mermaid::detail::Title> = QStringView{u"  title \""};
template<> constexpr auto suffix<mermaid::detail::FragmentType, mermaid::detail::Title> = QStringView{u"\""};
template<> constexpr auto prefix<mermaid::detail::SequenceType, mermaid::detail::XAxis> = QStringView{u"  x-axis [\""};
template<> constexpr auto joiner<mermaid::detail::SequenceType, mermaid::detail::XAxis> = QStringView{u"\", \""};
template<> constexpr auto suffix<mermaid::detail::SequenceType, mermaid::detail::XAxis> = QStringView{u"\"]"};
template<> constexpr auto prefix<mermaid::detail::FragmentType, mermaid::detail::YAxis> = QStringView{u"  y-axis \""};
template<> constexpr auto suffix<mermaid::detail::FragmentType, mermaid::detail::YAxis> = QStringView{u"\""};
template<> constexpr auto prefix<mermaid::detail::SequenceType, mermaid::detail::Line>  = QStringView{u"  line ["};
template<> constexpr auto joiner<mermaid::detail::SequenceType, mermaid::detail::Line>  = QStringView{u", "};
template<> constexpr auto suffix<mermaid::detail::SequenceType, mermaid::detail::Line>  = QStringView{u"]"};
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
