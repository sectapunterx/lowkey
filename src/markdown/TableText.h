#pragma once

#include <QChar>
#include <QList>
#include <QString>
#include <QStringList>

namespace heap::md {

// Delimited text as a table (APP-315): ```csv / ```tsv blocks, .csv / .tsv
// files opened or pasted into a note.

// Rows of `text` split at `separator` as RFC 4180 reads them: a field in
// double quotes may hold the separator and line breaks, "" is one quote.
// A blank last line is not a row. CRLF and LF both end a record.
QList<QStringList> parseDelimited(const QString& text, QChar separator);

// The separator a csv uses: the one of , ; | and tab found most often (and
// the same number of times) on its first lines, outside quotes. "," when
// none is.
QChar guessSeparator(const QString& text);

// The separator a fence's language asks for: "tsv" a tab, "csv" a guess
// from the text, anything else none (QChar()).
QChar separatorForLanguage(const QString& language, const QString& text);

// The rows as a markdown table, the first row its header, every row padded
// to the widest. A "|" is escaped and a line break becomes a space, so a cell
// stays one cell. At most `maxRows` rows after the header (0: all) and
// `maxCellChars` characters per cell (0: all, else cut with "…").
QString markdownTable(const QList<QStringList>& rows, int maxRows = 0, int maxCellChars = 0);

}  // namespace heap::md
