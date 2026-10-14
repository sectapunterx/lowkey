#pragma once

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>

namespace heap::md {

// Reading an .xlsx (APP-315): the cells' values, sheet by sheet, for the
// in-app viewer and for pasting a sheet into a note as a table. Values only:
// no formulas, styles, merged cells or charts; a date is its serial number.

struct XlsxSheet {
  QString name;
  QList<QStringList> rows;  // row-major, padded to the sheet's widest row
};

// The entries of a zip archive, by name: stored and deflated ones, which is
// all an .xlsx holds. Empty, with `error` set, when the data is not a zip it
// can read.
QHash<QString, QByteArray> unzip(const QByteArray& zip, QString* error = nullptr);

// The sheets of a workbook, in the workbook's order, at most `maxRows` rows
// each (0: all). Empty, with `error` set, when it cannot be read.
QList<XlsxSheet> readXlsx(const QByteArray& data, int maxRows = 0, QString* error = nullptr);

// "B3" -> column 1 (0-based), -1 for a reference with no letters.
int xlsxColumn(const QString& ref);

}  // namespace heap::md
