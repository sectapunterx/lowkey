#pragma once

#include <QDateTime>
#include <QString>
#include <QStringList>
#include <QStringView>
#include <QVector>

namespace heap::chrono {
class ChronoParser;
}

// What one line of task input means (APP-245/246/266): the dates, split into
// "when I do it" and "the deadline", and an estimate — each with where it sat
// in the line, so the input can mark it and a chip can give the words back.
// A pure function of the text, the parser and `now`.
namespace heap::capture {

// A recognised part of the line, as offsets into the text that was parsed.
struct Span {
  int start = -1;
  int end = -1;  // one past the last character
  QString kind;  // "when", "due", "remind", "estimate"
  QString text;  // the words, as typed (the deadline word included)

  bool operator==(const Span& o) const = default;
};

struct Parsed {
  // The line with the recognised parts cut out, whitespace collapsed.
  QString title;
  // A date with no deadline word: when the task is to be done.
  QDateTime when;
  bool whenHasTime = false;
  // The end of a time range said with it ("16:00-16:45"): a meeting's length.
  QDateTime whenEnd;
  // The zone the time was said in ("15:00 EST", "по Москве"), as an id
  // CalEvent::tz keeps; empty when none was. `when` is already local: a task
  // keeps no zone, a meeting may keep this one (A6-7).
  QString whenZone;
  // Already behind `now` ("вчера"): allowed, shown greyed.
  bool whenPast = false;
  // A date after a deadline word ("до пятницы", "к 15:00", "due fri", "by
  // monday"): the deadline.
  QDateTime due;
  bool dueHasTime = false;
  bool duePast = false;
  // A date after a reminder word ("напомни в 15:00", "remind me at 3pm",
  // APP-319): when to be reminded. Always with a clock time.
  QDateTime remind;
  // "~30m", "~1.5h", "~2ч"; 0 when none.
  int estimateMinutes = 0;
  QString recurrence;
  QVector<Span> spans;
};

// The words that turn the date right after them into a deadline.
const QStringList& deadlineWords();

// The words that turn the date right after them into a reminder ("напомни",
// "remind me"); "мне"/"me" may come between.
const QStringList& remindWords();

// Minutes in an estimate token ("~30m", "~1,5h", "~2ч", "~90мин", "~1h30m");
// 0 when `token` is not one.
int estimateMinutes(QStringView token);

// Parses `text`. The first date of each kind wins; any further dates stay in
// the title as words, so nothing is guessed from them. `rejected` lists
// recognised phrases (Span::text) the person took back with the chip's ×:
// they stay words in the title.
Parsed parse(const QString& text, const heap::chrono::ChronoParser& chrono, const QDateTime& now, const QStringList& rejected = {});

// The profile sigil (A5-21): "+Side project" names the profile a task goes
// to, by the profile's whole name, any case. '#' stays a one-word label. A
// '+word' that names no profile is just text.
struct ProfileSigil {
  int index = -1;  // into `names`; -1 = none
  int start = -1;  // the '+'
  int end = -1;    // one past the name
  QString text;    // as typed, '+' included
};

// The first sigil in the first line of `text`, before any "//" note: a '+'
// at the start or after a space, then the longest of `names` that follows,
// ending the word. Pure.
ProfileSigil findProfileSigil(const QString& text, const QStringList& names);

// `text` without the sigil, the spaces around it closed up.
QString withoutSigil(const QString& text, const ProfileSigil& sigil);

}  // namespace heap::capture
