#include "capture/CaptureParse.h"
#include "chrono/ChronoParser.h"

#include <QRegularExpression>

#include <algorithm>
#include <cmath>

namespace heap::capture {

const QStringList& deadlineWords() {
  // "до пятницы", "к понедельнику", "ко вторнику", "due fri", "by monday".
  static const QStringList kWords = {
      QStringLiteral("до"), QStringLiteral("к"), QStringLiteral("ко"), QStringLiteral("due"), QStringLiteral("by")};
  return kWords;
}

const QStringList& remindWords() {
  static const QStringList kWords = {QStringLiteral("напомни"),
                                     QStringLiteral("напомнить"),
                                     QStringLiteral("напомните"),
                                     QStringLiteral("remind"),
                                     QStringLiteral("reminder")};
  return kWords;
}

int estimateMinutes(QStringView token) {
  static const QRegularExpression kRe(
      QStringLiteral(
          R"(^~(?:(\d+(?:[.,]\d+)?)\s*(h|ч|час|часа|часов)\s*(?:(\d+)\s*(m|м|мин))?|(\d+)\s*(m|м|мин|минут|минуты|минута|min|mins|minutes))$)"),
      QRegularExpression::CaseInsensitiveOption);
  // Not matchView(): CI builds with Qt 6.9, which does not have it.
  const auto m = kRe.match(token.toString());
  if(!m.hasMatch()) {
    return 0;
  }
  if(m.hasCaptured(1)) {
    QString hours = m.captured(1);
    hours.replace(QLatin1Char(','), QLatin1Char('.'));
    const double h = hours.toDouble();
    const int extra = m.hasCaptured(3) ? m.captured(3).toInt() : 0;
    return static_cast<int>(std::lround(h * 60.0)) + extra;
  }
  return m.captured(5).toInt();
}

namespace {

// A length typed the way people say it, without the "~" (A1-23, A4-2):
// "25м", "25 мин", "за 50 минут", "на 1ч", "2ч", "1.5h", "на полчаса". A
// number after a word that makes it a moment ("через 30 мин", "в 2ч", "in
// 30 min") or before one ("2 часа дня", "5 min ago") is not a length.
struct BareEstimate {
  int start = -1;
  int end = -1;
  int minutes = 0;
};

// True when nothing but a date, a time or punctuation follows `from`: a bare
// length ends the line ("Сверстать хедер 25 мин", "тест 25м завтра"). In the
// middle of a title a number with a unit is part of what the task is called:
// "Fix 2h timeout", "Increase 30 min cache TTL" (REG-1).
bool onlyDatesFollow(const QString& text, int from, const heap::chrono::ChronoParser& chrono, const QDateTime& now) {
  QString rest = text.mid(from);
  for(const heap::chrono::ParseResult& r : chrono.parseAll(rest, now)) {
    if(!r.ok || r.startOffset < 0 || r.endOffset <= r.startOffset) {
      continue;
    }
    for(int i = r.startOffset; i < r.endOffset && i < rest.size(); ++i) {
      rest[i] = QLatin1Char(' ');
    }
  }
  static const QRegularExpression kWord(QStringLiteral(R"(\p{L}+)"), QRegularExpression::UseUnicodePropertiesOption);
  for(auto it = kWord.globalMatch(rest); it.hasNext();) {
    if(!deadlineWords().contains(it.next().captured().toLower())) {
      return false;
    }
  }
  return true;
}

BareEstimate bareEstimate(const QString& text, const heap::chrono::ChronoParser& chrono, const QDateTime& now) {
  // "оценка 2ч задача": a word that names the size makes the length one
  // anywhere in the line.
  static const QStringList kSizeWords = {
      QStringLiteral("оценка"), QStringLiteral("оценкой"), QStringLiteral("est"), QStringLiteral("estimate"), QStringLiteral("estimated")};
  static const QRegularExpression kRe(
      QStringLiteral(R"((?<![\p{L}\d~:.,])(?:(за|на|for)\s+)?)"
                     R"((?:(\d+(?:[.,]\d+)?)(?:(h|ч)|\s*(час|часа|часов|hours?|hrs?)(?!\p{L}))(?:\s*(\d+)\s*(?:m|м|мин)(?!\p{L}))?)"
                     R"(|(\d+)(?:(m|м)|\s*(мин|минут|минуты|минута|min|mins|minutes)(?!\p{L}))|(полчаса))(?![\p{L}\d]))"),
      QRegularExpression::CaseInsensitiveOption | QRegularExpression::UseUnicodePropertiesOption);
  static const QStringList kMomentBefore = {
      QStringLiteral("через"), QStringLiteral("in"),   QStringLiteral("в"),     QStringLiteral("во"),     QStringLiteral("к"),
      QStringLiteral("ко"),    QStringLiteral("до"),   QStringLiteral("с"),     QStringLiteral("со"),     QStringLiteral("at"),
      QStringLiteral("by"),    QStringLiteral("from"), QStringLiteral("to"),    QStringLiteral("after"),  QStringLiteral("после"),
      QStringLiteral("until"), QStringLiteral("till"), QStringLiteral("every"), QStringLiteral("каждые"), QStringLiteral("каждый"),
      QStringLiteral("каждую")};
  static const QStringList kMomentAfter = {QStringLiteral("назад"),
                                           QStringLiteral("ago"),
                                           QStringLiteral("дня"),
                                           QStringLiteral("ночи"),
                                           QStringLiteral("утра"),
                                           QStringLiteral("вечера"),
                                           QStringLiteral("am"),
                                           QStringLiteral("pm"),
                                           QStringLiteral("later"),
                                           QStringLiteral("спустя")};
  static const QRegularExpression kWordBefore(QStringLiteral(R"((\p{L}+)\s*$)"), QRegularExpression::UseUnicodePropertiesOption);
  static const QRegularExpression kWordAfter(QStringLiteral(R"(^\s*(\p{L}+))"), QRegularExpression::UseUnicodePropertiesOption);
  for(auto it = kRe.globalMatch(text); it.hasNext();) {
    const auto m = it.next();
    // "полчаса" alone is a word; "на/за полчаса" is the length.
    if(m.hasCaptured(9) && !m.hasCaptured(1)) {
      continue;
    }
    const auto before = kWordBefore.match(text.left(static_cast<int>(m.capturedStart())));
    const QString wordBefore = before.hasMatch() ? before.captured(1).toLower() : QString();
    if(!m.hasCaptured(1) && kMomentBefore.contains(wordBefore)) {
      continue;
    }
    if(!kSizeWords.contains(wordBefore) && !onlyDatesFollow(text, static_cast<int>(m.capturedEnd()), chrono, now)) {
      continue;
    }
    const auto after = kWordAfter.match(text.mid(static_cast<int>(m.capturedEnd())));
    if(after.hasMatch() && kMomentAfter.contains(after.captured(1).toLower())) {
      continue;
    }
    int minutes = 0;
    if(m.hasCaptured(9)) {
      minutes = 30;
    } else if(m.hasCaptured(2)) {
      QString hours = m.captured(2);
      hours.replace(QLatin1Char(','), QLatin1Char('.'));
      // "14ч" is a clock time, not fourteen hours of work.
      if(hours.toDouble() > 12.0) {
        continue;
      }
      minutes = static_cast<int>(std::lround(hours.toDouble() * 60.0)) + (m.hasCaptured(5) ? m.captured(5).toInt() : 0);
    } else {
      minutes = m.captured(6).toInt();
      if(minutes > 16 * 60) {
        continue;
      }
    }
    if(minutes <= 0) {
      continue;
    }
    return BareEstimate{static_cast<int>(m.capturedStart()), static_cast<int>(m.capturedEnd()), minutes};
  }
  return {};
}

// "в 9" / "at 9": a bare hour under 12 with no date and no am/pm. The parser
// takes the next 9:00, which after 9:00 is tomorrow morning; the nearer of
// 9:00 and 21:00 is what "at 9" usually means (APP-266).
QDateTime nearerBareHour(const heap::chrono::ParseResult& r, const QDateTime& now) {
  static const QRegularExpression kBare(QStringLiteral(R"(^\s*(?:в|at|к)?\s*(\d{1,2})\s*$)"), QRegularExpression::CaseInsensitiveOption);
  const auto m = kBare.match(r.consumed);
  if(!m.hasMatch() || !r.hasTime) {
    return r.start;
  }
  const int hour = m.captured(1).toInt();
  if(hour < 1 || hour > 11 || r.start.time().hour() != hour) {
    return r.start;
  }
  const QDateTime evening(now.date(), QTime(hour + 12, r.start.time().minute()));
  return evening > now && evening < r.start ? evening : r.start;
}

// "пт" said on a Friday is next Friday, not today (APP-266): a weekday is
// named to point away from today. "today"/"сегодня" and a bare time stay.
QDateTime weekdayNotToday(const heap::chrono::ParseResult& r, const QDateTime& now) {
  if(r.start.date() != now.date() || !r.recurrence.isEmpty()) {
    return r.start;
  }
  static const QRegularExpression kWeekday(
      QStringLiteral(R"((?<!\p{L})(пн|вт|ср|чт|пт|сб|вс|понед\p{L}*|вторн\p{L}*|сред\p{L}*|четв\p{L}*|пятн\p{L}*|субб\p{L}*|воскр\p{L}*|)"
                     // Whole English words (IDIOT-TASKS-7): "mon\p{L}*" took "month",
                     // and "end of month" on the 31st went a week ahead.
                     R"(mon|monday|tue|tues|tuesday|wed|weds|wednesday|thu|thur|thurs|thursday|)"
                     R"(fri|friday|sat|saturday|sun|sunday)(?!\p{L}))"),
      QRegularExpression::CaseInsensitiveOption | QRegularExpression::UseUnicodePropertiesOption);
  return kWeekday.match(r.consumed).hasMatch() ? r.start.addDays(7) : r.start;
}

bool isPast(const QDateTime& d, bool hasTime, const QDateTime& now) {
  return hasTime ? d < now : d.date() < now.date();
}

// The deadline word right before `at`, if one is there: its start offset,
// or -1.
int deadlineWordBefore(const QString& text, int at) {
  int end = at;
  while(end > 0 && text.at(end - 1).isSpace()) {
    --end;
  }
  int start = end;
  while(start > 0 && text.at(start - 1).isLetter()) {
    --start;
  }
  if(start == end || (start > 0 && !text.at(start - 1).isSpace() && !text.at(start - 1).isPunct())) {
    return -1;
  }
  const QString word = text.mid(start, end - start).toLower();
  return deadlineWords().contains(word) ? start : -1;
}

// The reminder word before `at` ("напомни [мне] в 15:00"), if one is there:
// its start offset, or -1.
int remindWordBefore(const QString& text, int at) {
  const auto wordBefore = [&](int pos, int* start) {
    int end = pos;
    while(end > 0 && text.at(end - 1).isSpace()) {
      --end;
    }
    int s = end;
    while(s > 0 && text.at(s - 1).isLetter()) {
      --s;
    }
    *start = s;
    return text.mid(s, end - s).toLower();
  };
  int start = -1;
  QString word = wordBefore(at, &start);
  if(word == QStringLiteral("мне") || word == QLatin1String("me")) {
    word = wordBefore(start, &start);
  }
  if(word.isEmpty() || (start > 0 && !text.at(start - 1).isSpace() && !text.at(start - 1).isPunct())) {
    return -1;
  }
  return remindWords().contains(word) ? start : -1;
}

}  // namespace

Parsed parse(const QString& text, const heap::chrono::ChronoParser& chrono, const QDateTime& now, const QStringList& rejected) {
  Parsed out;

  // ── the estimate: "~30m" as one token ──
  static const QRegularExpression kEstimate(QStringLiteral(R"((?<=^|\s)~\S+(?:\s*\d+\s*(?:m|м|мин))?(?=\s|$))"),
                                            QRegularExpression::CaseInsensitiveOption);
  for(auto it = kEstimate.globalMatch(text); it.hasNext();) {
    const auto m = it.next();
    const int minutes = estimateMinutes(m.capturedView());
    if(minutes <= 0 || rejected.contains(m.captured())) {
      continue;
    }
    out.estimateMinutes = minutes;
    out.spans.append(
        Span{static_cast<int>(m.capturedStart()), static_cast<int>(m.capturedEnd()), QStringLiteral("estimate"), m.captured()});
    break;
  }
  if(out.estimateMinutes == 0) {
    const BareEstimate b = bareEstimate(text, chrono, now);
    const QString words = b.minutes > 0 ? text.mid(b.start, b.end - b.start) : QString();
    if(b.minutes > 0 && !rejected.contains(words)) {
      out.estimateMinutes = b.minutes;
      out.spans.append(Span{b.start, b.end, QStringLiteral("estimate"), words});
    }
  }

  // ── dates ──
  // Read with the estimate blanked out, so "~2ч" is not read as a time.
  QString scan = text;
  for(const Span& s : out.spans) {
    for(int i = s.start; i < s.end; ++i) {
      scan[i] = QLatin1Char(' ');
    }
  }
  for(const heap::chrono::ParseResult& r : chrono.parseAll(scan, now)) {
    if(!r.ok || !r.start.isValid() || r.startOffset < 0 || r.endOffset <= r.startOffset) {
      continue;
    }
    // The deadline word before the date, or the parser's own first word
    // ("до пятницы" is one phrase to it).
    int marker = deadlineWordBefore(scan, r.startOffset);
    if(marker < 0) {
      for(const QString& w : deadlineWords()) {
        const int after = r.startOffset + static_cast<int>(w.size());
        if(after < r.endOffset && scan.mid(r.startOffset, w.size()).compare(w, Qt::CaseInsensitive) == 0 && scan.at(after).isSpace()) {
          marker = r.startOffset;
          break;
        }
      }
    }
    // A reminder takes a moment: a date with a clock time.
    const int remindMarker = marker < 0 && r.hasTime ? remindWordBefore(scan, r.startOffset) : -1;
    const bool isRemind = remindMarker >= 0 && !out.remind.isValid();
    const bool isDue = !isRemind && marker >= 0;
    const int from = isRemind ? remindMarker : isDue ? marker : r.startOffset;
    const QString words = text.mid(from, r.endOffset - from);
    if(rejected.contains(words)) {
      continue;
    }
    if(isRemind) {
      out.remind = weekdayNotToday(r, now);
      if(out.remind == r.start) {
        out.remind = nearerBareHour(r, now);
      }
      out.spans.append(Span{from, r.endOffset, QStringLiteral("remind"), words});
      continue;
    }
    if(isDue) {
      if(out.due.isValid()) {
        continue;
      }
      out.due = weekdayNotToday(r, now);
      out.dueHasTime = r.hasTime;
      out.duePast = isPast(out.due, r.hasTime, now);
    } else {
      if(out.when.isValid()) {
        continue;
      }
      out.when = weekdayNotToday(r, now);
      if(out.when == r.start) {
        out.when = nearerBareHour(r, now);
      }
      out.whenHasTime = r.hasTime;
      out.whenZone = r.zone;
      out.whenPast = isPast(out.when, r.hasTime, now);
      if(r.end.isValid() && r.end > r.start) {
        out.whenEnd = r.end;
      } else if(r.end.isValid() && r.hasTime && r.end.time() < r.start.time()) {
        // "22:00-01:00" ends the next morning (IDIOT-CAL-11); the end was
        // dropped and the meeting got the default length.
        out.whenEnd = QDateTime(out.when.date().addDays(1), r.end.time());
      }
    }
    if(!r.recurrence.isEmpty() && out.recurrence.isEmpty()) {
      out.recurrence = r.recurrence;
    }
    out.spans.append(Span{from, r.endOffset, isDue ? QStringLiteral("due") : QStringLiteral("when"), words});
  }

  std::sort(out.spans.begin(), out.spans.end(), [](const Span& a, const Span& b) {
    return a.start < b.start;
  });
  QString title;
  int at = 0;
  for(const Span& s : out.spans) {
    title += text.mid(at, s.start - at);
    title += QLatin1Char(' ');
    at = s.end;
  }
  title += text.mid(at);
  out.title = title.simplified();
  // A cut leaves its comma behind: "do it , then" reads "do it, then".
  out.title.replace(QStringLiteral(" ,"), QStringLiteral(","));
  while(out.title.startsWith(QLatin1Char(',')) || out.title.startsWith(QLatin1Char('-'))) {
    out.title = out.title.mid(1).trimmed();
  }
  // A comma or dash left dangling where a date was cut out ("call Ann, ").
  while(!out.title.isEmpty() && (out.title.endsWith(QLatin1Char(',')) || out.title.endsWith(QLatin1Char('-')))) {
    out.title.chop(1);
    out.title = out.title.trimmed();
  }
  return out;
}

ProfileSigil findProfileSigil(const QString& text, const QStringList& names) {
  ProfileSigil out;
  qsizetype lineEnd = text.indexOf(QLatin1Char('\n'));
  if(lineEnd < 0) {
    lineEnd = text.size();
  }
  const qsizetype note = text.left(lineEnd).indexOf(QStringLiteral("//"));
  if(note >= 0) {
    lineEnd = note;
  }
  const auto isWordChar = [](QChar c) {
    return c.isLetterOrNumber() || c == QLatin1Char('_') || c == QLatin1Char('-');
  };
  for(qsizetype at = text.indexOf(QLatin1Char('+')); at >= 0 && at < lineEnd; at = text.indexOf(QLatin1Char('+'), at + 1)) {
    if(at > 0 && !text.at(at - 1).isSpace()) {
      continue;
    }
    int best = -1;
    qsizetype bestLen = 0;
    for(int i = 0; i < names.size(); ++i) {
      const QString name = names.at(i).trimmed();
      if(name.isEmpty() || name.size() <= bestLen || at + 1 + name.size() > lineEnd) {
        continue;
      }
      if(QStringView(text).mid(at + 1, name.size()).compare(name, Qt::CaseInsensitive) != 0) {
        continue;
      }
      const qsizetype after = at + 1 + name.size();
      if(after < text.size() && isWordChar(text.at(after))) {
        continue;
      }
      best = i;
      bestLen = name.size();
    }
    if(best >= 0) {
      out.index = best;
      out.start = static_cast<int>(at);
      out.end = static_cast<int>(at + 1 + bestLen);
      out.text = text.mid(at, 1 + bestLen);
      return out;
    }
  }
  return out;
}

QString withoutSigil(const QString& text, const ProfileSigil& sigil) {
  if(sigil.index < 0 || sigil.start < 0) {
    return text;
  }
  QString left = text.left(sigil.start);
  QString right = text.mid(sigil.end);
  while(left.endsWith(QLatin1Char(' '))) {
    left.chop(1);
  }
  while(right.startsWith(QLatin1Char(' '))) {
    right.remove(0, 1);
  }
  if(left.isEmpty() || right.isEmpty() || right.startsWith(QLatin1Char('\n'))) {
    return left + right;
  }
  return left + QLatin1Char(' ') + right;
}

}  // namespace heap::capture
