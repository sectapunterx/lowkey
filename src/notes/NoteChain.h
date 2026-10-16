#pragma once

#include <QDateTime>
#include <QRegularExpression>
#include <QString>
#include <QStringList>

// Quick notes that run on (APP-320): a capture made within an hour of the
// last note goes on in that note; after a longer pause, or under another
// label, it starts a new one. Pure functions of what they are handed and
// `now`, so the rule is tested without a clock.
namespace heap::notes {

inline constexpr int kChainMinutes = 60;

// Labels compare as tags do: without the "#", case-insensitively.
inline QString normLabel(const QString& label) {
  QString l = label.trimmed();
  while(l.startsWith(QLatin1Char('#'))) {
    l.remove(0, 1);
  }
  return l.toLower();
}

// Whether a capture labelled `label` at `now` goes on in the note last
// written at `lastAt` under `lastLabel`.
inline bool chainAppends(const QDateTime& lastAt, const QString& lastLabel, const QString& label, const QDateTime& now) {
  if(!lastAt.isValid() || !now.isValid() || now < lastAt) {
    return false;
  }
  return lastAt.secsTo(now) < 60LL * kChainMinutes && normLabel(lastLabel) == normLabel(label);
}

// The #tags of a note's text, as the editor draws them (MdHtml): lower case,
// each once, in order. "#HEAP-12" is a ticket, not a tag.
inline QStringList noteTags(const QString& body) {
  static const QRegularExpression kTicket(QStringLiteral(R"((?<![A-Za-z0-9_])#[A-Z][A-Z0-9]*-\d+)"));
  static const QRegularExpression kTag(QStringLiteral(R"((?<![\p{L}\p{N}_])#([\p{L}][\p{L}\p{N}_/-]*))"),
                                       QRegularExpression::UseUnicodePropertiesOption);
  QString text = body;
  text.replace(kTicket, QStringLiteral(" "));
  QStringList out;
  for(auto it = kTag.globalMatch(text); it.hasNext();) {
    const QString t = it.next().captured(1).toLower();
    if(!out.contains(t)) {
      out << t;
    }
  }
  return out;
}

// A new note's title from the first words written into it: the first line
// with text, without list or heading marks and tags, at most `max` letters,
// cut at a word.
inline QString titleFromEntry(const QString& text, int max = 60) {
  static const QRegularExpression kMarks(QStringLiteral(R"(^\s*(?:#{1,6}\s+|[-*+]\s+(?:\[[ xX]\]\s+)?|\d+[.)]\s+|>\s*)*)"));
  static const QRegularExpression kTags(QStringLiteral(R"((?<![\p{L}\p{N}_])#[\p{L}][\p{L}\p{N}_/-]*)"),
                                        QRegularExpression::UseUnicodePropertiesOption);
  for(QString line : text.split(QLatin1Char('\n'))) {
    line.remove(kMarks);
    line.remove(kTags);
    line = line.simplified();
    if(line.isEmpty()) {
      continue;
    }
    if(line.size() <= max) {
      return line;
    }
    const int cut = line.lastIndexOf(QLatin1Char(' '), max);
    return line.left(cut > max / 2 ? cut : max).trimmed() + QStringLiteral("…");
  }
  return {};
}

}  // namespace heap::notes
