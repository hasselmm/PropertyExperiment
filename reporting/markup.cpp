#include "markup.h"

namespace reporting {
namespace {

auto length(QStringView text)
{
    return text.length();
}

auto maximumLength(const auto &range)
{
    return std::ranges::max(range | std::views::transform(length));
}

} // namespace

QString TableColumn::alignText(const QString &text) const
{
    const auto available = width - text.length();

    if (available <= 0)
        return text;

    if (align & Qt::AlignRight)
        return text.rightJustified(width);
    else if (align & Qt::AlignCenter)
        return text.leftJustified(text.length() + available/2).rightJustified(width);
    else
        return text.leftJustified(width);
}

void TableHeader::addColumns(QStringList titles, Qt::Alignment align)
{
    std::ranges::transform(titles, std::back_inserter(columns), [align](QString title) {
        return TableColumn{std::move(title), align};
    });
}

void TableHeader::updateColumnWidth(int row, QStringList values)
{
    columns[row].width = std::max(columns[row].width, maximumLength(values));
}

QString TableHeader::makeTitle(const TableColumn &column) noexcept
{
    return column.title.leftJustified(column.width);
}

QString TableHeader::makeUnderline(const TableColumn &column) noexcept
{
    auto underline = QString{column.width, u'-'};

    if (column.align & Qt::AlignHCenter)
        underline.front() = underline.back() = u':';
    else if (column.align & Qt::AlignLeft)
        underline.front() = u':';
    else if (column.align & Qt::AlignRight)
        underline.back() = u':';

    return underline;
}

} // namespace reporting
