// One line of task input, split into "when" and "due" (APP-245) and an
// estimate (APP-246). A pure function of the text and `now`.
#include "capture/CaptureParse.h"
#include "chrono/ChronoParser.h"
#include "recur/RecurrenceEngine.h"

#include <QDate>
#include <QDateTime>
#include <QLocale>
#include <QTime>
#include <QTimeZone>

#include <gtest/gtest.h>

using heap::capture::parse;
using heap::capture::Parsed;

namespace {

const QDateTime kNow(QDate(2026, 5, 20), QTime(10, 0));  // Wednesday 10:00

class Capture : public ::testing::Test {
 protected:
  heap::chrono::ChronoParser chrono{QLocale(QLocale::Russian, QLocale::Russia)};

  Parsed p(const QString& text, const QStringList& rejected = {}) {
    return parse(text, chrono, kNow, rejected);
  }
};

}  // namespace

TEST_F(Capture, ADateWithNoDeadlineWordIsWhen) {
  const Parsed r = p(QStringLiteral("позвонить Ане завтра в 15:00"));
  EXPECT_EQ(r.title, QStringLiteral("позвонить Ане"));
  EXPECT_EQ(r.when, QDateTime(QDate(2026, 5, 21), QTime(15, 0)));
  EXPECT_TRUE(r.whenHasTime);
  EXPECT_FALSE(r.due.isValid()) << "one date is no longer both";
}

TEST_F(Capture, ADateAfterDoIsTheDeadline) {
  const Parsed r = p(QStringLiteral("отчёт до пятницы"));
  EXPECT_EQ(r.title, QStringLiteral("отчёт"));
  EXPECT_EQ(r.due.date(), QDate(2026, 5, 22));
  EXPECT_FALSE(r.dueHasTime);
  EXPECT_FALSE(r.when.isValid());
  ASSERT_EQ(r.spans.size(), 1);
  EXPECT_EQ(r.spans.at(0).kind, QStringLiteral("due"));
  EXPECT_EQ(r.spans.at(0).text, QStringLiteral("до пятницы"));
}

TEST_F(Capture, WhenAndDueInOneLine) {
  const Parsed r = p(QStringLiteral("сделать в понедельник, сдать до пятницы"));
  EXPECT_EQ(r.when.date(), QDate(2026, 5, 25));
  EXPECT_EQ(r.due.date(), QDate(2026, 5, 22));
  EXPECT_EQ(r.title, QStringLiteral("сделать, сдать"));
}

TEST_F(Capture, TheDeadlineMayComeFirst) {
  const Parsed r = p(QStringLiteral("до пт ревью, завтра начать"));
  EXPECT_EQ(r.due.date(), QDate(2026, 5, 22));
  EXPECT_EQ(r.when.date(), QDate(2026, 5, 21));
  EXPECT_EQ(r.title, QStringLiteral("ревью, начать"));
}

TEST_F(Capture, EnglishDueAndBy) {
  heap::chrono::ChronoParser en{QLocale(QLocale::English, QLocale::UnitedStates)};
  const Parsed a = parse(QStringLiteral("write spec due fri"), en, kNow);
  EXPECT_EQ(a.title, QStringLiteral("write spec"));
  EXPECT_EQ(a.due.date(), QDate(2026, 5, 22));
  const Parsed b = parse(QStringLiteral("tomorrow 9am start, ship by monday"), en, kNow);
  EXPECT_EQ(b.when, QDateTime(QDate(2026, 5, 21), QTime(9, 0)));
  EXPECT_EQ(b.due.date(), QDate(2026, 5, 25));
}

TEST_F(Capture, ADeadlineWordInsideAWordIsNotOne) {
  // "Подробно" ends in "до", "Скоро" is not "к": only a whole word counts.
  const Parsed r = p(QStringLiteral("разобрать подробно завтра"));
  EXPECT_TRUE(r.when.isValid());
  EXPECT_FALSE(r.due.isValid());
}

TEST_F(Capture, ASecondUnmarkedDateStaysInTheTitle) {
  const Parsed r = p(QStringLiteral("перенести с завтра на пятницу"));
  EXPECT_TRUE(r.when.isValid());
  EXPECT_FALSE(r.due.isValid());
  EXPECT_EQ(r.spans.size(), 1) << "nothing is guessed from the second date";
}

TEST_F(Capture, ARejectedPhraseStaysWords) {
  const Parsed first = p(QStringLiteral("Обзор пятницы"));
  ASSERT_EQ(first.spans.size(), 1);
  const Parsed again = p(QStringLiteral("Обзор пятницы"), {first.spans.at(0).text});
  EXPECT_EQ(again.title, QStringLiteral("Обзор пятницы"));
  EXPECT_FALSE(again.when.isValid());
  EXPECT_TRUE(again.spans.isEmpty());
}

TEST_F(Capture, ATypoIsNotGuessed) {
  const Parsed r = p(QStringLiteral("купить хлеб завтар"));
  EXPECT_EQ(r.title, QStringLiteral("купить хлеб завтар"));
  EXPECT_FALSE(r.when.isValid());
}

TEST_F(Capture, EstimateTokens) {
  using heap::capture::estimateMinutes;
  EXPECT_EQ(estimateMinutes(u"~30m"), 30);
  EXPECT_EQ(estimateMinutes(u"~45мин"), 45);
  EXPECT_EQ(estimateMinutes(u"~1.5h"), 90);
  EXPECT_EQ(estimateMinutes(u"~1,5ч"), 90);
  EXPECT_EQ(estimateMinutes(u"~2ч"), 120);
  EXPECT_EQ(estimateMinutes(u"~1h30m"), 90);
  EXPECT_EQ(estimateMinutes(u"~soon"), 0);
  EXPECT_EQ(estimateMinutes(u"30m"), 0) << "the tilde is the marker";
}

TEST_F(Capture, AnEstimateIsCutOutAndNotReadAsATime) {
  const Parsed r = p(QStringLiteral("ревью PR ~2ч завтра"));
  EXPECT_EQ(r.estimateMinutes, 120);
  EXPECT_EQ(r.title, QStringLiteral("ревью PR"));
  EXPECT_EQ(r.when.date(), QDate(2026, 5, 21));
  EXPECT_FALSE(r.whenHasTime) << "\"2ч\" is the estimate, not two o'clock";
}

TEST_F(Capture, EmptyAndPlainText) {
  EXPECT_TRUE(p(QString()).title.isEmpty());
  const Parsed r = p(QStringLiteral("просто задача"));
  EXPECT_EQ(r.title, QStringLiteral("просто задача"));
  EXPECT_TRUE(r.spans.isEmpty());
}

TEST_F(Capture, ABareHourIsTheNearerOfMorningAndEvening) {
  // 10:00 now: "в 9" is 21:00 today, not 9:00 tomorrow.
  const Parsed r = p(QStringLiteral("созвон в 9"));
  EXPECT_EQ(r.when, QDateTime(QDate(2026, 5, 20), QTime(21, 0)));
  // "в 11" is still ahead this morning.
  EXPECT_EQ(p(QStringLiteral("созвон в 11")).when, QDateTime(QDate(2026, 5, 20), QTime(11, 0)));
  // A date said with it is taken as said.
  EXPECT_EQ(p(QStringLiteral("созвон завтра в 9")).when, QDateTime(QDate(2026, 5, 21), QTime(9, 0)));
}

TEST_F(Capture, APastDateIsAllowedAndFlagged) {
  const Parsed r = p(QStringLiteral("отчёт вчера"));
  EXPECT_EQ(r.when.date(), QDate(2026, 5, 19));
  EXPECT_TRUE(r.whenPast);
  EXPECT_FALSE(p(QStringLiteral("отчёт завтра")).whenPast);
}

TEST_F(Capture, AWeekdayNamedOnThatDayIsNextWeek) {
  const QDateTime friday(QDate(2026, 5, 22), QTime(10, 0));
  EXPECT_EQ(parse(QStringLiteral("отчёт до пятницы"), chrono, friday).due.date(), QDate(2026, 5, 29));
  EXPECT_EQ(parse(QStringLiteral("ревью в пт"), chrono, friday).when.date(), QDate(2026, 5, 29));
  EXPECT_EQ(parse(QStringLiteral("ревью сегодня"), chrono, friday).when.date(), QDate(2026, 5, 22)) << "today is today";
  EXPECT_EQ(parse(QStringLiteral("ревью в 15:00"), chrono, friday).when.date(), QDate(2026, 5, 22)) << "a bare time is today";
}

// ── The 0.8.4 acceptance run (IDIOT-TASKS-5…8) ──

namespace {
Parsed at(const QString& text, const QDateTime& now) {
  static heap::chrono::ChronoParser en{QLocale(QLocale::English, QLocale::UnitedStates)};
  return parse(text, en, now, {});
}
}  // namespace

TEST(CaptureAcceptance, TheFifteenthIsADayOfTheMonthNotThursday) {
  const Parsed r = at(QStringLiteral("pay rent on the 15th"), QDateTime(QDate(2026, 1, 31), QTime(12, 0)));
  EXPECT_EQ(r.when.date(), QDate(2026, 2, 15));
  EXPECT_EQ(r.title, QStringLiteral("pay rent"));
  const Parsed late = at(QStringLiteral("pay rent on the 15th"), QDateTime(QDate(2026, 12, 31), QTime(12, 0)));
  EXPECT_EQ(late.when.date(), QDate(2027, 1, 15));
}

TEST(CaptureAcceptance, ADayAndMonthAlreadyPastIsNextYears) {
  const QDateTime nye(QDate(2026, 12, 31), QTime(12, 0));
  EXPECT_EQ(at(QStringLiteral("25.10 release"), nye).when.date(), QDate(2027, 10, 25));
  EXPECT_EQ(at(QStringLiteral("release 25 october"), nye).when.date(), QDate(2027, 10, 25));
  EXPECT_EQ(at(QStringLiteral("release 25.10.2026"), nye).when.date(), QDate(2026, 10, 25)) << "a typed year is taken as said";
}

TEST(CaptureAcceptance, EndOfMonthOnTheLastDayIsNotAWeekLater) {
  const Parsed r = at(QStringLiteral("end of month invoice"), QDateTime(QDate(2026, 1, 31), QTime(12, 0)));
  EXPECT_LT(r.when.date(), QDate(2026, 2, 7));
}

TEST(CaptureAcceptance, ThisSaturdayTakesThisAndTwoHoursIsNoClock) {
  const QDateTime sat(QDate(2026, 10, 10), QTime(12, 0));
  EXPECT_EQ(at(QStringLiteral("clean this sat"), sat).title, QStringLiteral("clean"));
  const Parsed est = at(QStringLiteral("estimate 2h fix"), sat);
  EXPECT_FALSE(est.whenHasTime && est.when.time() == QTime(2, 0));
}

TEST(CaptureAcceptance, ARangePastMidnightEndsTheNextMorning) {
  // IDIOT-CAL-11.
  const Parsed r = at(QStringLiteral("release 22:00-01:00"), QDateTime(QDate(2026, 10, 10), QTime(12, 0)));
  EXPECT_EQ(r.when, QDateTime(QDate(2026, 10, 10), QTime(22, 0)));
  EXPECT_EQ(r.whenEnd, QDateTime(QDate(2026, 10, 11), QTime(1, 0)));
}

// ── 0.9 acceptance (APP-221, batch B2) ────────────────────────────────────

// A3-1, A4-1: the half of the day as a Russian word after the hour.
TEST(CaptureAcceptance, RussianHalfOfTheDayWords) {
  const QDateTime d1441(QDate(2026, 10, 12), QTime(14, 41));
  Parsed r = at(QStringLiteral("проверить логи в 3 ночи"), d1441);
  EXPECT_EQ(r.title, QStringLiteral("проверить логи"));
  EXPECT_EQ(r.when, QDateTime(QDate(2026, 10, 13), QTime(3, 0))) << "3 am has passed: the next one";
  r = at(QStringLiteral("в 3 утра деплой"), d1441);
  EXPECT_EQ(r.title, QStringLiteral("деплой"));
  EXPECT_EQ(r.when, QDateTime(QDate(2026, 10, 13), QTime(3, 0)));
  r = at(QStringLiteral("в 12 ночи бэкап"), d1441);
  EXPECT_EQ(r.title, QStringLiteral("бэкап"));
  EXPECT_EQ(r.when, QDateTime(QDate(2026, 10, 13), QTime(0, 0))) << "12 at night is midnight, not noon";
  r = at(QStringLiteral("завтра в 2 ночи бэкап"), d1441);
  EXPECT_EQ(r.title, QStringLiteral("бэкап"));
  EXPECT_EQ(r.when, QDateTime(QDate(2026, 10, 13), QTime(2, 0)));

  const QDateTime d1453(QDate(2026, 10, 12), QTime(14, 53));
  r = at(QStringLiteral("созвон в 9 утра"), d1453);
  EXPECT_EQ(r.title, QStringLiteral("созвон"));
  EXPECT_EQ(r.when, QDateTime(QDate(2026, 10, 13), QTime(9, 0))) << "not 21:00 today";
  r = at(QStringLiteral("созвон в 11 вечера"), QDateTime(QDate(2026, 10, 12), QTime(7, 0)));
  EXPECT_EQ(r.when, QDateTime(QDate(2026, 10, 12), QTime(23, 0)));
  r = at(QStringLiteral("созвон в 7 вечера"), QDateTime(QDate(2026, 10, 12), QTime(7, 0)));
  EXPECT_EQ(r.when, QDateTime(QDate(2026, 10, 12), QTime(19, 0)));
  r = at(QStringLiteral("созвон в 2 часа дня"), QDateTime(QDate(2026, 10, 12), QTime(7, 0)));
  EXPECT_EQ(r.title, QStringLiteral("созвон"));
  EXPECT_EQ(r.when, QDateTime(QDate(2026, 10, 12), QTime(14, 0)));
  r = at(QStringLiteral("созвон в 3 дня"), QDateTime(QDate(2026, 10, 12), QTime(22, 0)));
  EXPECT_EQ(r.when, QDateTime(QDate(2026, 10, 13), QTime(15, 0)));
  r = at(QStringLiteral("созвон в 3 часа ночи"), d1453);
  EXPECT_EQ(r.title, QStringLiteral("созвон"));
  EXPECT_EQ(r.when, QDateTime(QDate(2026, 10, 13), QTime(3, 0)));
  r = at(QStringLiteral("завтра в 3 дня позвонить маме"), d1453);
  EXPECT_EQ(r.title, QStringLiteral("позвонить маме"));
  EXPECT_EQ(r.when, QDateTime(QDate(2026, 10, 13), QTime(15, 0)));
  // The same words that are not about the clock stay words.
  EXPECT_FALSE(at(QStringLiteral("отпуск 3 дня"), d1453).when.isValid());
  EXPECT_FALSE(at(QStringLiteral("3 ночи в отеле"), d1453).when.isValid());
}

// A1-23, A4-2: a length said without the "~".
TEST(CaptureAcceptance, LengthsWithoutTheTilde) {
  const QDateTime now(QDate(2026, 10, 12), QTime(17, 0));
  Parsed r = at(QStringLiteral("Починить тест завтра 10:00 25м"), now);
  EXPECT_EQ(r.title, QStringLiteral("Починить тест"));
  EXPECT_EQ(r.estimateMinutes, 25);
  EXPECT_EQ(r.when, QDateTime(QDate(2026, 10, 13), QTime(10, 0)));
  r = at(QStringLiteral("Сверстать хедер 25 мин"), now);
  EXPECT_EQ(r.title, QStringLiteral("Сверстать хедер"));
  EXPECT_EQ(r.estimateMinutes, 25);
  r = at(QStringLiteral("Ревью PR за 50 минут"), now);
  EXPECT_EQ(r.title, QStringLiteral("Ревью PR"));
  EXPECT_EQ(r.estimateMinutes, 50);
  r = at(QStringLiteral("Адаптив футера на 2 помидора"), now);
  EXPECT_FALSE(r.when.isValid()) << "two tomatoes are not 02:00";
  EXPECT_EQ(r.title, QStringLiteral("Адаптив футера на 2 помидора"));
  r = at(QStringLiteral("оценка 2ч задача"), now);
  EXPECT_FALSE(r.when.isValid());
  EXPECT_EQ(r.estimateMinutes, 120);
  EXPECT_EQ(r.title, QStringLiteral("оценка задача"));
  r = at(QStringLiteral("задача на 1ч"), now);
  EXPECT_FALSE(r.when.isValid());
  EXPECT_EQ(r.estimateMinutes, 60);
  EXPECT_EQ(r.title, QStringLiteral("задача"));
  r = at(QStringLiteral("созвон на полчаса"), now);
  EXPECT_EQ(r.estimateMinutes, 30);
  EXPECT_EQ(r.title, QStringLiteral("созвон"));
  EXPECT_EQ(at(QStringLiteral("созвон 30 мин"), now).estimateMinutes, 30);
  EXPECT_EQ(at(QStringLiteral("Сторибук ~25m"), now).estimateMinutes, 25);
  // A moment is not a length.
  r = at(QStringLiteral("через 30 мин позвонить"), now);
  EXPECT_EQ(r.estimateMinutes, 0);
  EXPECT_EQ(r.when, QDateTime(QDate(2026, 10, 12), QTime(17, 30)));
  r = at(QStringLiteral("созвон в 2ч"), now);
  EXPECT_EQ(r.estimateMinutes, 0);
  EXPECT_TRUE(r.whenHasTime);
  r = at(QStringLiteral("встреча 14ч"), now);
  EXPECT_EQ(r.estimateMinutes, 0);
  EXPECT_EQ(r.when.time(), QTime(14, 0));
  EXPECT_EQ(at(QStringLiteral("созвон в 2 часа дня"), now).estimateMinutes, 0);
  EXPECT_EQ(at(QStringLiteral("с 2 до 3 часов дня"), now).estimateMinutes, 0);
  // REG-1: a number with a unit inside a title is part of its name.
  for(const char* title : {"Fix 2h timeout", "Increase 30 min cache TTL", "Fix 404 on 3m page", "Поднять лимит 5 мин для джобы"}) {
    const QString text = QString::fromUtf8(title);
    const Parsed w = at(text, now);
    EXPECT_EQ(w.estimateMinutes, 0) << title;
    EXPECT_EQ(w.title, text) << title;
  }
  r = at(QStringLiteral("Починить тест 25м завтра"), now);
  EXPECT_EQ(r.estimateMinutes, 25);
  EXPECT_EQ(r.title, QStringLiteral("Починить тест"));
}

// A3-2: the clock wins over the part of the day named with it.
TEST(CaptureAcceptance, TheClockBeatsTonight) {
  const QDateTime now(QDate(2026, 10, 12), QTime(10, 0));
  Parsed r = at(QStringLiteral("tonight 11pm deploy"), now);
  EXPECT_EQ(r.title, QStringLiteral("deploy"));
  EXPECT_EQ(r.when, QDateTime(QDate(2026, 10, 12), QTime(23, 0)));
  r = at(QStringLiteral("11pm tonight deploy"), now);
  EXPECT_EQ(r.title, QStringLiteral("deploy"));
  EXPECT_EQ(r.when, QDateTime(QDate(2026, 10, 12), QTime(23, 0)));
  r = at(QStringLiteral("deploy tonight at 11"), now);
  EXPECT_EQ(r.title, QStringLiteral("deploy"));
  EXPECT_EQ(r.when, QDateTime(QDate(2026, 10, 12), QTime(23, 0)));
  r = at(QStringLiteral("созвон вечером в 7"), now);
  EXPECT_EQ(r.title, QStringLiteral("созвон"));
  EXPECT_EQ(r.when, QDateTime(QDate(2026, 10, 12), QTime(19, 0)));
}

// A6-1, A6-2: each end of a range keeps its own half of the day.
TEST(CaptureAcceptance, RangesWithTwoMeridiems) {
  const QDateTime now(QDate(2026, 10, 12), QTime(8, 0));
  Parsed r = at(QStringLiteral("workshop 9am-5pm"), now);
  EXPECT_EQ(r.when, QDateTime(QDate(2026, 10, 12), QTime(9, 0)));
  EXPECT_EQ(r.whenEnd, QDateTime(QDate(2026, 10, 12), QTime(17, 0)));
  r = at(QStringLiteral("lunch 11am-1pm"), now);
  EXPECT_EQ(r.when.time(), QTime(11, 0));
  EXPECT_EQ(r.whenEnd.time(), QTime(13, 0));
  r = at(QStringLiteral("deploy 11pm-1am"), now);
  EXPECT_EQ(r.when, QDateTime(QDate(2026, 10, 12), QTime(23, 0)));
  EXPECT_EQ(r.whenEnd, QDateTime(QDate(2026, 10, 13), QTime(1, 0)));
  r = at(QStringLiteral("call 2-3pm"), now);
  EXPECT_EQ(r.when.time(), QTime(14, 0));
  EXPECT_EQ(r.whenEnd.time(), QTime(15, 0));
  r = at(QStringLiteral("call 11-1pm"), now);
  EXPECT_EQ(r.when.time(), QTime(11, 0));
  EXPECT_EQ(r.whenEnd.time(), QTime(13, 0));
  // A backwards slip is not a 23-hour meeting.
  EXPECT_FALSE(at(QStringLiteral("call 11:00-10:00"), now).whenEnd.isValid());
  r = at(QStringLiteral("late sync 23:30-00:30"), now);
  EXPECT_EQ(r.when, QDateTime(QDate(2026, 10, 12), QTime(23, 30)));
  EXPECT_EQ(r.whenEnd, QDateTime(QDate(2026, 10, 13), QTime(0, 30)));
}

// A6-6: a zone said with the time is that zone's time.
TEST(CaptureAcceptance, AZoneSaidWithTheTime) {
  const QDateTime now(QDate(2026, 10, 12), QTime(10, 0));
  Parsed r = at(QStringLiteral("Standup tomorrow 3pm EST"), now);
  EXPECT_EQ(r.title, QStringLiteral("Standup"));
  // F1-new-3: EST is New York with its DST, as in the zone picker.
  EXPECT_EQ(r.when, QDateTime(QDate(2026, 10, 13), QTime(15, 0), QTimeZone("America/New_York")));
  r = at(QStringLiteral("sync tue 15:00 UTC-5"), now);
  EXPECT_EQ(r.title, QStringLiteral("sync"));
  EXPECT_EQ(r.when, QDateTime(QDate(2026, 10, 13), QTime(15, 0), QTimeZone(-5 * 3600)));
  r = at(QStringLiteral("созвон завтра в 19:00 по мск"), now);
  EXPECT_EQ(r.title, QStringLiteral("созвон"));
  EXPECT_EQ(r.when, QDateTime(QDate(2026, 10, 13), QTime(19, 0), QTimeZone(3 * 3600)));
  r = at(QStringLiteral("call fri 19:00 New York"), now);
  EXPECT_EQ(r.title, QStringLiteral("call"));
  EXPECT_EQ(r.when, QDateTime(QDate(2026, 10, 16), QTime(19, 0), QTimeZone("America/New_York")));
  // Lower-case "est" is a word, not a zone.
  EXPECT_EQ(at(QStringLiteral("tomorrow 3pm est"), now).title, QStringLiteral("est"));
  r = at(QStringLiteral("call tue 15:00 GMT-5"), now);
  EXPECT_EQ(r.title, QStringLiteral("call"));
  EXPECT_EQ(r.when, QDateTime(QDate(2026, 10, 13), QTime(15, 0), QTimeZone(-5 * 3600)));
  r = at(QStringLiteral("call tue 15:00 UTC-05:00"), now);
  EXPECT_EQ(r.title, QStringLiteral("call"));
  EXPECT_EQ(r.when, QDateTime(QDate(2026, 10, 13), QTime(15, 0), QTimeZone(-5 * 3600)));
  // "ET" / "PT" follow the season (EDT in October).
  r = at(QStringLiteral("review tomorrow 3pm ET"), now);
  EXPECT_EQ(r.title, QStringLiteral("review"));
  EXPECT_EQ(r.when, QDateTime(QDate(2026, 10, 13), QTime(15, 0), QTimeZone("America/New_York")));
  r = at(QStringLiteral("review tomorrow 3pm PT"), now);
  EXPECT_EQ(r.title, QStringLiteral("review"));
  EXPECT_EQ(r.when, QDateTime(QDate(2026, 10, 13), QTime(15, 0), QTimeZone("America/Los_Angeles")));
}

// A6-7: the zone said is kept by name, so a meeting can stay in it. The
// time itself is already local (above); a task keeps only that.
TEST(CaptureAcceptance, TheZoneSaidIsNamed) {
  const QDateTime now(QDate(2026, 10, 12), QTime(10, 0));
  // F1-new-3: a short name is its region (with DST), the same as the picker;
  // EST, EDT and ET give one hour on one day. An offset stays an offset.
  EXPECT_EQ(at(QStringLiteral("Standup tomorrow 3pm EST"), now).whenZone, QStringLiteral("America/New_York"));
  EXPECT_EQ(at(QStringLiteral("Standup tomorrow 3pm EDT"), now).whenZone, QStringLiteral("America/New_York"));
  EXPECT_EQ(at(QStringLiteral("Sync tomorrow 10am PST"), now).whenZone, QStringLiteral("America/Los_Angeles"));
  EXPECT_EQ(at(QStringLiteral("Sync tomorrow 10am CST"), now).whenZone, QStringLiteral("America/Chicago"));
  EXPECT_EQ(at(QStringLiteral("Sync tomorrow 10am MST"), now).whenZone, QStringLiteral("America/Denver"));
  EXPECT_EQ(at(QStringLiteral("Sync tomorrow 10am CET"), now).whenZone, QStringLiteral("Europe/Berlin"));
  EXPECT_EQ(at(QStringLiteral("Vendor call tomorrow 15:00 EST"), now).when.toUTC(),
            at(QStringLiteral("Vendor call tomorrow 15:00 ET"), now).when.toUTC());
  EXPECT_EQ(at(QStringLiteral("Vendor call tomorrow 15:00 EST"), now).when.toUTC(),
            QDateTime(QDate(2026, 10, 13), QTime(19, 0), QTimeZone::utc()))
      << "15:00 in New York in October (EDT) is 19:00 UTC";
  EXPECT_EQ(at(QStringLiteral("call tue 15:00 UTC-5"), now).whenZone, QStringLiteral("UTC-05:00"));
  EXPECT_EQ(at(QStringLiteral("review tomorrow 3pm PT"), now).whenZone, QStringLiteral("America/Los_Angeles"));
  Parsed r = at(QStringLiteral("sync tomorrow 10:00 New York"), now);
  EXPECT_EQ(r.title, QStringLiteral("sync"));
  EXPECT_EQ(r.whenZone, QStringLiteral("America/New_York"));
  EXPECT_EQ(r.when, QDateTime(QDate(2026, 10, 13), QTime(10, 0), QTimeZone("America/New_York")));
  r = at(QString::fromUtf8("созвон завтра в 19:00 по Москве"), now);
  EXPECT_EQ(r.title, QString::fromUtf8("созвон"));
  EXPECT_EQ(r.whenZone, QStringLiteral("Europe/Moscow"));
  EXPECT_EQ(r.when, QDateTime(QDate(2026, 10, 13), QTime(19, 0), QTimeZone("Europe/Moscow")));
  r = at(QString::fromUtf8("созвон завтра в 19:00 по мск"), now);
  EXPECT_EQ(r.whenZone, QStringLiteral("Europe/Moscow")) << "MSK is Moscow, not a bare +3";
  r = at(QString::fromUtf8("звонок пт 18:00 по Нью-Йорку"), now);
  EXPECT_EQ(r.title, QString::fromUtf8("звонок"));
  EXPECT_EQ(r.whenZone, QStringLiteral("America/New_York"));
  r = at(QStringLiteral("call tue 15:00 UTC+5"), now);
  EXPECT_EQ(r.whenZone, QStringLiteral("UTC+05:00"));
  EXPECT_EQ(r.when, QDateTime(QDate(2026, 10, 13), QTime(15, 0), QTimeZone(5 * 3600)));
  r = at(QStringLiteral("call 15:00 in London"), now);
  EXPECT_EQ(r.title, QStringLiteral("call"));
  EXPECT_EQ(r.whenZone, QStringLiteral("Europe/London"));
  // No zone said, none named; a city in lower case without "in" is a word.
  EXPECT_TRUE(at(QStringLiteral("call tomorrow 15:00"), now).whenZone.isEmpty());
  r = at(QStringLiteral("demo tomorrow 15:00 boston office"), now);
  EXPECT_TRUE(r.whenZone.isEmpty());
  EXPECT_EQ(r.title, QStringLiteral("demo boston office"));
}

// A2-20, A5-4, A4-22: habitual repeats, and the ones the app has no rule
// for stay whole in the title instead of being half read.
TEST(CaptureAcceptance, HabitualRepeats) {
  const QDateTime now(QDate(2026, 10, 12), QTime(10, 0));  // Monday
  Parsed r = at(QStringLiteral("buy milk daily"), now);
  EXPECT_EQ(r.recurrence, QStringLiteral("every:day"));
  EXPECT_EQ(r.title, QStringLiteral("buy milk"));
  EXPECT_EQ(at(QStringLiteral("report weekly"), now).recurrence, QStringLiteral("every:week"));
  EXPECT_EQ(at(QStringLiteral("standup daily at 10am"), now).recurrence, QStringLiteral("every:day"));
  // REG-1: an adjective before a noun is part of the title.
  for(const char* adjective : {"Fix daily standup bot", "Prepare daily standup notes", "weekly report", "Send monthly invoice"}) {
    const QString text = QString::fromUtf8(adjective);
    const Parsed w = at(text, now);
    EXPECT_EQ(w.title, text) << adjective;
    EXPECT_TRUE(w.recurrence.isEmpty()) << adjective;
  }
  EXPECT_EQ(at(QStringLiteral("Проверить логи ежедневно"), now).recurrence, QStringLiteral("every:day"));
  EXPECT_EQ(at(QStringLiteral("отчёт еженедельно"), now).recurrence, QStringLiteral("every:week"));
  r = at(QStringLiteral("Бэкап раз в неделю"), now);
  EXPECT_EQ(r.recurrence, QStringLiteral("every:week"));
  EXPECT_EQ(r.title, QStringLiteral("Бэкап"));
  r = at(QStringLiteral("Ротация ключей каждое 1-е число"), now);
  EXPECT_EQ(r.recurrence, QStringLiteral("every:month:1"));
  EXPECT_EQ(r.title, QStringLiteral("Ротация ключей"));
  EXPECT_EQ(r.when.date(), QDate(2026, 11, 1));
  r = at(QStringLiteral("pay rent 1st of every month"), now);
  EXPECT_EQ(r.recurrence, QStringLiteral("every:month:1"));
  EXPECT_EQ(r.title, QStringLiteral("pay rent"));
  r = at(QStringLiteral("pay rent on the 15th of each month"), now);
  EXPECT_EQ(r.recurrence, QStringLiteral("every:month:15"));
  EXPECT_EQ(r.title, QStringLiteral("pay rent"));
}

// B7: interval repeats and lists of days are read now; B2 left them whole
// in the title. Monday 12 October 2026.
TEST(CaptureAcceptance, IntervalRepeats) {
  const QDateTime now(QDate(2026, 10, 12), QTime(10, 0));  // Monday

  struct Case {
    const char* text;
    const char* title;
    const char* rule;
    QDate when;
  };

  const Case cases[] = {
      {"water plants every other day", "water plants", "every:2:day", QDate(2026, 10, 12)},
      {"water plants every 2 days", "water plants", "every:2:day", QDate(2026, 10, 12)},
      {"sync every 2 weeks", "sync", "every:2:week", QDate(2026, 10, 12)},
      {"retro every 3 weeks", "retro", "every:3:week", QDate(2026, 10, 12)},
      {"invoice every 2 months", "invoice", "every:2:month", QDate(2026, 10, 12)},
      {"every other friday sync", "sync", "every:2:week", QDate(2026, 10, 16)},
      {"sync every 2 weeks on friday", "sync", "every:2:week", QDate(2026, 10, 16)},
      {"Buy milk every mon, wed", "Buy milk", "every:mon,wed", QDate(2026, 10, 12)},
      {"gym every tue and thu", "gym", "every:thu,tue", QDate(2026, 10, 13)},
      {"standup every weekday", "standup", "every:weekday", QDate(2026, 10, 12)},
      {"Проверить диски каждые 2 дня", "Проверить диски", "every:2:day", QDate(2026, 10, 12)},
      {"полив через день", "полив", "every:2:day", QDate(2026, 10, 12)},
      {"ретро каждые 2 недели", "ретро", "every:2:week", QDate(2026, 10, 12)},
      {"ретро каждые две недели", "ретро", "every:2:week", QDate(2026, 10, 12)},
      {"отчёт каждую вторую неделю", "отчёт", "every:2:week", QDate(2026, 10, 12)},
      {"бэкап раз в 2 недели", "бэкап", "every:2:week", QDate(2026, 10, 12)},
      {"счёт каждые 3 месяца", "счёт", "every:3:month", QDate(2026, 10, 12)},
      {"стендап каждый пн и ср", "стендап", "every:mon,wed", QDate(2026, 10, 12)},
      {"зал по пн, ср, пт", "зал", "every:mon,wed,fri", QDate(2026, 10, 12)},
      {"зал по вторникам и четвергам", "зал", "every:thu,tue", QDate(2026, 10, 13)},
      {"отчёт по пн, вт, ср, чт, пт", "отчёт", "every:weekday", QDate(2026, 10, 12)},
  };
  for(const Case& c : cases) {
    const Parsed r = at(QString::fromUtf8(c.text), now);
    // The rule as the engine writes it: days in week order.
    EXPECT_EQ(r.recurrence, heap::recur::canonicalRule(QString::fromUtf8(c.rule))) << c.text;
    EXPECT_EQ(r.title, QString::fromUtf8(c.title)) << c.text;
    EXPECT_EQ(r.when.date(), c.when) << c.text;
  }
  // A time with it stays the time; the first day still ahead.
  Parsed r = at(QStringLiteral("standup every mon, wed 9:00"), now);
  EXPECT_EQ(r.recurrence, QStringLiteral("every:mon,wed"));
  EXPECT_EQ(r.when, QDateTime(QDate(2026, 10, 14), QTime(9, 0))) << "Monday 9:00 is behind 10:00";
  // Not a list: a two-letter English word after "and" may be a word, so
  // the phrase stays whole, as before lists were read.
  r = at(QStringLiteral("call every mon and we sync"), now);
  EXPECT_TRUE(r.recurrence.isEmpty());
  EXPECT_EQ(r.title, QStringLiteral("call every mon and we sync"));
  // An offset is still an offset: "через 2 дня" is Wednesday, once.
  r = at(QString::fromUtf8("позвонить через 2 дня"), now);
  EXPECT_TRUE(r.recurrence.isEmpty());
  EXPECT_EQ(r.when.date(), QDate(2026, 10, 14));
  // A day of the month is still one.
  r = at(QString::fromUtf8("аренда каждое 25 число"), now);
  EXPECT_EQ(r.recurrence, QStringLiteral("every:month:25"));
  // An interval no one plans with stays words.
  r = at(QStringLiteral("ping every 900 days"), now);
  EXPECT_TRUE(r.recurrence.isEmpty());
  // "по 2 дня" is a share, not a repeat.
  r = at(QString::fromUtf8("дежурства по 2 дня"), now);
  EXPECT_TRUE(r.recurrence.isEmpty());
}

// F1-new-5: a list of days without commas, and the day said before the
// repeat, are the same rule as the comma list after it. Monday 12 Oct 2026.
TEST(CaptureAcceptance, DayListsWithoutCommasAndDaysBeforeTheRule) {
  const QDateTime now(QDate(2026, 10, 12), QTime(10, 0));  // Monday

  struct Case {
    const char* text;
    const char* title;
    const char* rule;
    QDate when;
  };

  const Case cases[] = {
      {"Gym every mon wed fri", "Gym", "every:mon,wed,fri", QDate(2026, 10, 12)},
      {"every mon wed fri gym", "gym", "every:mon,wed,fri", QDate(2026, 10, 12)},
      {"Gym mon and wed every week", "Gym", "every:mon,wed", QDate(2026, 10, 12)},
      {"Gym tue thu weekly", "Gym", "every:tue,thu", QDate(2026, 10, 13)},
      {"Sync fri every 2 weeks", "Sync", "every:2:week", QDate(2026, 10, 16)},
      {"Pay rent 15th every 2 months", "Pay rent", "every:2:month:15", QDate(2026, 10, 15)},
      {"Pay rent 1st every month", "Pay rent", "every:month:1", QDate(2026, 11, 1)},
      {"зал пн ср пт каждую неделю", "зал", "every:mon,wed,fri", QDate(2026, 10, 12)},
      {"аренда 15-го каждый месяц", "аренда", "every:month:15", QDate(2026, 10, 15)},
      {"стендап каждый пн ср", "стендап", "every:mon,wed", QDate(2026, 10, 12)},
  };
  for(const Case& c : cases) {
    const Parsed r = at(QString::fromUtf8(c.text), now);
    EXPECT_EQ(r.recurrence, heap::recur::canonicalRule(QString::fromUtf8(c.rule))) << c.text;
    EXPECT_EQ(r.title, QString::fromUtf8(c.title)) << c.text;
    EXPECT_EQ(r.when.date(), c.when) << c.text;
  }
  // Still words: a two-letter English day, and a list before an interval
  // (as after one: there is no rule for it).
  Parsed r = at(QStringLiteral("call every mon we sync"), now);
  EXPECT_EQ(r.recurrence, QStringLiteral("every:mon"));
  EXPECT_EQ(r.title, QStringLiteral("call we sync"));
  r = at(QStringLiteral("Review mon"), now);
  EXPECT_TRUE(r.recurrence.isEmpty()) << "a day alone is a date";
  EXPECT_EQ(r.title, QStringLiteral("Review"));
}

// B7 follow-up: a month interval from a date or on a day of the month, the
// lists an interval cannot carry, and the phrases that only look like one.
// Monday 12 October 2026.
TEST(CaptureAcceptance, IntervalRepeatsFollowUp) {
  const QDateTime now(QDate(2026, 10, 12), QTime(10, 0));  // Monday

  struct Case {
    const char* text;
    const char* title;
    const char* rule;
    QDate when;
  };

  const Case cases[] = {
      // A start date is the first one, not an interval after it.
      {"invoice every 2 months from 15 nov", "invoice", "every:2:month", QDate(2026, 11, 15)},
      {"invoice every 2 months from 1 nov", "invoice", "every:2:month", QDate(2026, 11, 1)},
      {"invoice every 3 months from 1 nov", "invoice", "every:3:month", QDate(2026, 11, 1)},
      {"sync every 2 weeks from 19 oct", "sync", "every:2:week", QDate(2026, 10, 19)},
      // A day of the month with the interval.
      {"rent every 2 months on the 15th", "rent", "every:2:month:15", QDate(2026, 10, 15)},
      {"rent every 2 months on the 15th from 1 nov", "rent", "every:2:month:15", QDate(2026, 11, 15)},
      {"аренда раз в 2 месяца 15-го", "аренда", "every:2:month:15", QDate(2026, 10, 15)},
      {"аренда каждые 2 месяца 15 числа", "аренда", "every:2:month:15", QDate(2026, 10, 15)},
      {"налог каждые 3 месяца 1-го", "налог", "every:3:month:1", QDate(2026, 11, 1)},
      // A day of the month on its own, with an English ordinal.
      {"pay every 15th", "pay", "every:month:15", QDate(2026, 10, 15)},
      {"pay every 4th", "pay", "every:month:4", QDate(2026, 11, 4)},
      {"pay every 15th of the month", "pay", "every:month:15", QDate(2026, 10, 15)},
      {"pay every 20th", "pay", "every:month:20", QDate(2026, 10, 20)},
      {"pay every 27th", "pay", "every:month:27", QDate(2026, 10, 27)},
      {"Fix every 2 th tests", "Fix tests", "every:month:2", QDate(2026, 11, 2)},
      // A list after a plain weekly repeat is the list.
      {"sync every week on mon, wed", "sync", "every:mon,wed", QDate(2026, 10, 12)},
  };
  for(const Case& c : cases) {
    const Parsed r = at(QString::fromUtf8(c.text), now);
    EXPECT_EQ(r.recurrence, heap::recur::canonicalRule(QString::fromUtf8(c.rule))) << c.text;
    EXPECT_EQ(r.title, QString::fromUtf8(c.title)) << c.text;
    EXPECT_EQ(r.when.date(), c.when) << c.text;
  }
  // A list of days after an interval has no rule: all of it stays words,
  // nothing half read (no Wednesday lost, no date).
  for(const char* whole : {"every 2 weeks on mon, wed design",
                           "every other mon, wed sync",
                           "every 2 weeks mon and wed review",
                           "бэкап раз в 2 недели по пн и ср",
                           "ретро каждые 2 недели в пн и ср"}) {
    const QString text = QString::fromUtf8(whole);
    const Parsed w = at(text, now);
    EXPECT_TRUE(w.recurrence.isEmpty()) << whole;
    EXPECT_EQ(w.title, text) << whole;
    EXPECT_FALSE(w.when.isValid()) << whole;
  }
  // "через день после X" is a day after something: once, tomorrow.
  Parsed r = at(QString::fromUtf8("Отписаться через день после релиза"), now);
  EXPECT_TRUE(r.recurrence.isEmpty());
  EXPECT_EQ(r.when.date(), QDate(2026, 10, 13));
}

// REG-3: a plain repeat takes in the day, time or start that follows it,
// or the whole phrase stays in the title; never only the repeat word.
TEST(CaptureAcceptance, ARepeatReadsTheDayAndStartAfterIt) {
  const QDateTime now(QDate(2026, 10, 13), QTime(10, 0));  // Tuesday
  Parsed r = at(QStringLiteral("report weekly on monday"), now);
  EXPECT_EQ(r.title, QStringLiteral("report"));
  EXPECT_EQ(r.recurrence, QStringLiteral("every:mon"));
  EXPECT_EQ(r.when.date(), QDate(2026, 10, 19));
  r = at(QStringLiteral("report every week on monday"), now);
  EXPECT_EQ(r.title, QStringLiteral("report"));
  EXPECT_EQ(r.recurrence, QStringLiteral("every:mon"));
  r = at(QString::fromUtf8("отчёт еженедельно по понедельникам"), now);
  EXPECT_EQ(r.title, QString::fromUtf8("отчёт"));
  EXPECT_EQ(r.recurrence, QStringLiteral("every:mon"));
  EXPECT_EQ(r.when.date(), QDate(2026, 10, 19));
  r = at(QString::fromUtf8("отчёт каждую неделю в понедельник"), now);
  EXPECT_EQ(r.title, QString::fromUtf8("отчёт"));
  EXPECT_EQ(r.recurrence, QStringLiteral("every:mon"));
  r = at(QString::fromUtf8("бэкап раз в неделю в пятницу 18:00"), now);
  EXPECT_EQ(r.title, QString::fromUtf8("бэкап"));
  EXPECT_EQ(r.recurrence, QStringLiteral("every:fri"));
  EXPECT_EQ(r.when, QDateTime(QDate(2026, 10, 16), QTime(18, 0)));
  r = at(QString::fromUtf8("стендап ежедневно с завтра"), now);
  EXPECT_EQ(r.title, QString::fromUtf8("стендап"));
  EXPECT_EQ(r.recurrence, QStringLiteral("every:day"));
  EXPECT_EQ(r.when.date(), QDate(2026, 10, 14));
  r = at(QStringLiteral("standup daily from tomorrow"), now);
  EXPECT_EQ(r.title, QStringLiteral("standup"));
  EXPECT_EQ(r.recurrence, QStringLiteral("every:day"));
  EXPECT_EQ(r.when.date(), QDate(2026, 10, 14));
  r = at(QStringLiteral("standup daily starting tomorrow at 9am"), now);
  EXPECT_EQ(r.title, QStringLiteral("standup"));
  EXPECT_EQ(r.when, QDateTime(QDate(2026, 10, 14), QTime(9, 0)));
  r = at(QStringLiteral("pay rent monthly on the 1st"), now);
  EXPECT_EQ(r.title, QStringLiteral("pay rent"));
  EXPECT_EQ(r.recurrence, QStringLiteral("every:month:1"));
  EXPECT_EQ(r.when.date(), QDate(2026, 11, 1));
  // A time range after "from" is a clock, not the 9th of the month.
  r = at(QStringLiteral("standup daily from 9 to 10"), now);
  EXPECT_EQ(r.recurrence, QStringLiteral("every:day"));
  EXPECT_EQ(r.when.time(), QTime(9, 0));
  EXPECT_EQ(r.title, QStringLiteral("standup"));
  r = at(QStringLiteral("weekly on monday report"), now);
  EXPECT_EQ(r.title, QStringLiteral("report"));
  EXPECT_EQ(r.recurrence, QStringLiteral("every:mon"));
  r = at(QStringLiteral("report weekly, monday"), now);
  EXPECT_EQ(r.title, QStringLiteral("report"));
  EXPECT_EQ(r.recurrence, QStringLiteral("every:mon"));
  // "on" with nothing readable after it: the whole phrase is the title.
  r = at(QStringLiteral("report weekly on call"), now);
  EXPECT_EQ(r.title, QStringLiteral("report weekly on call"));
  EXPECT_TRUE(r.recurrence.isEmpty());
  // Unchanged: a time after the repeat, and the repeat alone.
  r = at(QStringLiteral("report weekly at 10"), now);
  EXPECT_EQ(r.recurrence, QStringLiteral("every:week"));
  EXPECT_EQ(r.when.time(), QTime(10, 0));
  EXPECT_EQ(at(QString::fromUtf8("Бэкап раз в неделю"), now).recurrence, QStringLiteral("every:week"));
}

// A4-22.
TEST(CaptureAcceptance, NextWeekOnADayAndTheComingFriday) {
  const QDateTime now(QDate(2026, 10, 12), QTime(10, 0));  // Monday
  Parsed r = at(QStringLiteral("созвон на следующей неделе в пн"), now);
  EXPECT_EQ(r.title, QStringLiteral("созвон"));
  EXPECT_EQ(r.when.date(), QDate(2026, 10, 19));
  r = at(QStringLiteral("созвон на следующей неделе в среду"), now);
  EXPECT_EQ(r.when.date(), QDate(2026, 10, 21));
  r = at(QStringLiteral("demo next week on friday"), now);
  EXPECT_EQ(r.title, QStringLiteral("demo"));
  EXPECT_EQ(r.when.date(), QDate(2026, 10, 23));
  r = at(QStringLiteral("созвон в ближайшую пятницу"), now);
  EXPECT_EQ(r.title, QStringLiteral("созвон"));
  EXPECT_EQ(r.when.date(), QDate(2026, 10, 16));
}

// "напомни [мне] в 15:00" (APP-319): a reminder, not the plan; it leaves the
// title, and the plan said in the same line stays the plan.
TEST_F(Capture, ARemindWordMakesTheTimeAReminder) {
  const Parsed r = p(QStringLiteral("позвонить в банк напомни в 15:00"));
  EXPECT_EQ(r.title, QStringLiteral("позвонить в банк"));
  EXPECT_EQ(r.remind, QDateTime(kNow.date(), QTime(15, 0)));
  EXPECT_FALSE(r.when.isValid());
  const Parsed both = p(QStringLiteral("релиз завтра в 12:00 напомни мне в 11:30"));
  EXPECT_EQ(both.when, QDateTime(kNow.date().addDays(1), QTime(12, 0)));
  EXPECT_EQ(both.remind.time(), QTime(11, 30));
}

TEST_F(Capture, ARemindWordWithNoTimeIsJustAWord) {
  const Parsed r = p(QStringLiteral("напомни Ане про отчёт завтра"));
  EXPECT_FALSE(r.remind.isValid()) << "a reminder needs a clock time";
  EXPECT_EQ(r.when.date(), kNow.date().addDays(1));
}

TEST(CaptureEn, RemindMeAtThreeIsAReminder) {
  const heap::chrono::ChronoParser chrono{QLocale(QLocale::English, QLocale::UnitedStates)};
  const Parsed r = parse(QStringLiteral("call the bank remind me at 3pm"), chrono, kNow, {});
  EXPECT_EQ(r.remind, QDateTime(kNow.date(), QTime(15, 0)));
  EXPECT_EQ(r.title, QStringLiteral("call the bank"));
}
