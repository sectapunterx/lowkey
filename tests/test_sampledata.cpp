// Seed content sanity (HEAP-56).
//
// Guards two properties of the demo profile: calendar events are spread across
// the week (not all stamped to today, which left the Week view looking empty),
// and the sample content is generic software-dev material rather than the old
// niche telecom/LTE workload no non-telecom user could relate to.

#include "Models.h"
#include "SampleData.h"

#include <QColor>
#include <QDate>
#include <QRegularExpression>
#include <QSet>
#include <QStringList>

#include <gtest/gtest.h>

TEST(SampleData, EventsSpreadAcrossMultipleDays) {
  const QDate today(2026, 5, 19);
  const QVector<CalEvent> evs = SampleData::events(today, SampleData::Lang::En);
  ASSERT_FALSE(evs.isEmpty());

  QSet<qint64> days;
  for(const CalEvent& e : evs) {
    ASSERT_TRUE(e.date.isValid());
    days.insert(e.date.toJulianDay());
    EXPECT_GE(e.date.toJulianDay(), today.toJulianDay());
    EXPECT_LE(e.date.toJulianDay(), today.addDays(6).toJulianDay());
  }
  // At least three distinct days so Week / non-today views are populated.
  EXPECT_GE(days.size(), 3);
}

TEST(SampleData, SeedIsGenericNotTelecom) {
  for(const SampleData::Lang lang : {SampleData::Lang::En, SampleData::Lang::Ru}) {
    for(const Task& t : SampleData::tasks(lang)) {
      const QString blob = t.id + " " + t.title + " " + t.desc;
      EXPECT_FALSE(blob.contains("LTE")) << blob.toStdString();
      EXPECT_FALSE(blob.contains("PDCP"));
      EXPECT_FALSE(blob.contains("HARQ"));
      EXPECT_FALSE(blob.contains("3GPP"));
      EXPECT_FALSE(blob.contains("S1AP"));
    }
  }
}

TEST(SampleData, BoardStaysPopulatedAcrossColumns) {
  // A believable demo needs cards in several columns, not one pile.
  const QVector<Task> ts = SampleData::tasks(SampleData::Lang::En);
  QSet<QString> statuses;
  for(const Task& t : ts) {
    statuses.insert(t.status);
  }
  EXPECT_GE(ts.size(), 10);
  EXPECT_GE(statuses.size(), 4);
}

TEST(SampleData, DefaultStatusesHaveDistinctColours) {
  // Design audit DES-7: To Do and Code Review shipped the same grey, In
  // Progress and 50/50 the same blue, so the dot beside a card told two
  // statuses apart by nothing. A new profile gets one colour per status.
  for(const SampleData::Lang lang : {SampleData::Lang::En, SampleData::Lang::Ru}) {
    QSet<QString> colours;
    const QVector<QVariantMap> sts = SampleData::statuses(lang);
    for(const QVariantMap& s : sts) {
      colours.insert(s.value(QStringLiteral("color")).value<QColor>().name());
    }
    EXPECT_EQ(colours.size(), sts.size());
  }
}

namespace {

const SampleData::Lang kLangs[] = {SampleData::Lang::En, SampleData::Lang::Ru};

const Task* findTask(const QVector<Task>& ts, const QString& id) {
  for(const Task& t : ts) {
    if(t.id == id) {
      return &t;
    }
  }
  return nullptr;
}

}  // namespace

// APP-271: within a glance the example shows a developer's day: meetings,
// tasks planned between them, something in progress, overdue, due today and
// blocked, every priority, labels, a checklist, a relation, and a task whose
// plan and deadline are on different days.
TEST(SampleData, ExampleIsABelievableDay) {
  const QDate today = QDate::currentDate();
  for(const SampleData::Lang lang : kLangs) {
    const QVector<Task> ts = SampleData::tasks(lang);
    int timedToday = 0;
    int dayOnlyToday = 0;
    bool overdue = false;
    bool dueToday = false;
    bool planDiffersFromDue = false;
    bool checklist = false;
    bool relation = false;
    QSet<QString> statuses;
    QSet<QString> priorities;
    QSet<QString> ids;
    for(const Task& t : ts) {
      EXPECT_FALSE(ids.contains(t.id)) << t.id.toStdString();
      ids.insert(t.id);
      statuses.insert(t.status);
      priorities.insert(t.priority);
      if(t.scheduledAt.isValid() && t.scheduledAt.date() == today) {
        (t.scheduledHasTime ? timedToday : dayOnlyToday)++;
      }
      overdue = overdue || (t.dueAt.isValid() && t.dueAt.date() < today && t.status != QLatin1String("done"));
      dueToday = dueToday || (t.dueAt.isValid() && t.dueAt.date() == today);
      planDiffersFromDue = planDiffersFromDue || (t.scheduledAt.isValid() && t.dueAt.isValid() && t.scheduledAt.date() < t.dueAt.date());
      checklist = checklist || !t.local.checklist.isEmpty();
      relation = relation || !t.links.isEmpty();
      // The timer's total is the sum of its sessions.
      int sessions = 0;
      for(const TimerSession& s : t.local.sessions) {
        sessions += static_cast<int>(s.start.secsTo(s.end));
      }
      EXPECT_EQ(t.trackedSeconds, sessions) << t.id.toStdString();
    }
    EXPECT_GE(timedToday, 3);
    EXPECT_GE(dayOnlyToday, 1);
    EXPECT_TRUE(overdue);
    EXPECT_TRUE(dueToday);
    EXPECT_TRUE(planDiffersFromDue);
    EXPECT_TRUE(checklist);
    EXPECT_TRUE(relation);
    for(const char* s : {"backlog", "todo", "prog", "half", "blocked", "review", "done"}) {
      EXPECT_TRUE(statuses.contains(QLatin1String(s))) << s;
    }
    for(const char* p : {"P0", "P1", "P2", "P3"}) {
      EXPECT_TRUE(priorities.contains(QLatin1String(p))) << p;
    }

    // Today: three or four meetings, a standup, a 1:1 and a review/planning.
    QSet<QString> types;
    int meetingsToday = 0;
    for(const CalEvent& e : SampleData::events(today, lang)) {
      if(e.date == today) {
        ++meetingsToday;
        types.insert(e.type);
      }
    }
    EXPECT_GE(meetingsToday, 3);
    EXPECT_LE(meetingsToday, 4);
    EXPECT_TRUE(types.contains(QStringLiteral("standup")));
    EXPECT_TRUE(types.contains(QStringLiteral("oneone")));
    EXPECT_TRUE(types.contains(QStringLiteral("sync")));
  }
}

// Nothing in the example may look like a mirrored issue to the sync code: a
// task with a provider would be pushed, dismissed or marked gone upstream.
TEST(SampleData, ExampleTasksAreLocal) {
  for(const SampleData::Lang lang : kLangs) {
    for(const Task& t : SampleData::tasks(lang)) {
      EXPECT_TRUE(t.externalProvider.isEmpty()) << t.id.toStdString();
      EXPECT_TRUE(t.externalId.isEmpty()) << t.id.toStdString();
      EXPECT_TRUE(t.externalUrl.isEmpty()) << t.id.toStdString();
      EXPECT_TRUE(t.assignee.isEmpty()) << t.id.toStdString();
      EXPECT_FALSE(t.timerStartedAt.isValid()) << "a timer left running " << t.id.toStdString();
    }
  }
}

// Every #KEY-1 in a note or description, every relation and every wait names
// a task (and person) of the example; both languages are the same example.
TEST(SampleData, ExampleReferencesResolve) {
  static const QRegularExpression ref(QStringLiteral("(?<![A-Za-z0-9_])#([A-Z][A-Z0-9]*-\\d+)"));
  const QVector<Task> en = SampleData::tasks(SampleData::Lang::En);
  const QVector<Task> ru = SampleData::tasks(SampleData::Lang::Ru);
  ASSERT_EQ(en.size(), ru.size());
  for(int i = 0; i < en.size(); ++i) {
    EXPECT_EQ(en[i].id, ru[i].id);
    EXPECT_EQ(en[i].status, ru[i].status);
    EXPECT_EQ(en[i].priority, ru[i].priority);
    EXPECT_EQ(en[i].scheduledAt, ru[i].scheduledAt);
    EXPECT_EQ(en[i].dueAt, ru[i].dueAt);
    EXPECT_EQ(en[i].links, ru[i].links);
    EXPECT_EQ(en[i].labels, ru[i].labels);
  }
  for(const SampleData::Lang lang : kLangs) {
    const QVector<Task> ts = SampleData::tasks(lang);
    QStringList texts;
    for(const Task& t : ts) {
      texts << t.desc;
      for(const TaskLink& l : t.links) {
        EXPECT_NE(findTask(ts, l.targetId), nullptr) << l.targetId.toStdString();
      }
    }
    for(const Note& n : SampleData::notes(lang)) {
      texts << n.body;
    }
    for(const QString& text : texts) {
      for(auto it = ref.globalMatch(text); it.hasNext();) {
        const QString id = it.next().captured(1);
        EXPECT_NE(findTask(ts, id), nullptr) << id.toStdString();
      }
    }
    QSet<QString> people;
    for(const Person& p : SampleData::people(lang)) {
      people.insert(p.id);
    }
    for(const WaitingOn& w : SampleData::waitingOn()) {
      EXPECT_NE(findTask(ts, w.taskId), nullptr);
      EXPECT_TRUE(people.contains(w.personId));
      EXPECT_TRUE(w.remindedAt.isValid()) << "the example must not send a reminder of its own";
    }
    for(const CalEvent& e : SampleData::events(QDate::currentDate(), lang)) {
      if(!e.taskId.isEmpty()) {
        EXPECT_NE(findTask(ts, e.taskId), nullptr) << e.taskId.toStdString();
      }
    }
    // The guide is first, pinned, and shows the capture lines verbatim.
    const QVector<Note> notes = SampleData::notes(lang);
    ASSERT_FALSE(notes.isEmpty());
    EXPECT_TRUE(notes.constFirst().pinned);
    for(const QString& line : SampleData::captureExamples(lang)) {
      EXPECT_TRUE(notes.constFirst().body.contains(line)) << line.toStdString();
    }
  }
}

// Brand voice: calm and factual, no exclamation marks, no emoji, none of the
// words the brand does not use.
TEST(SampleData, ExampleKeepsTheBrandVoice) {
  static const QRegularExpression banned(QStringLiteral("productiv|\\b10x\\b|\\bAI\\b|seamless|продуктивн|бесшовн"),
                                         QRegularExpression::CaseInsensitiveOption);
  for(const SampleData::Lang lang : kLangs) {
    QStringList texts;
    for(const Task& t : SampleData::tasks(lang)) {
      texts << t.title << t.desc;
      for(const LocalCheckItem& c : t.local.checklist) {
        texts << c.text;
      }
    }
    for(const Note& n : SampleData::notes(lang)) {
      texts << n.title << n.body;
    }
    for(const CalEvent& e : SampleData::events(QDate::currentDate(), lang)) {
      texts << e.title << e.attendees;
    }
    for(const Person& p : SampleData::people(lang)) {
      texts << p.name << p.question;
    }
    for(const QString& text : texts) {
      EXPECT_FALSE(text.contains(QLatin1Char('!'))) << text.toStdString();
      EXPECT_FALSE(banned.match(text).hasMatch()) << text.toStdString();
      for(const QChar c : text) {
        EXPECT_FALSE(c.isSurrogate()) << "emoji in " << text.toStdString();
      }
      EXPECT_FALSE(text.contains(QStringLiteral("lowkey"), Qt::CaseInsensitive)) << text.toStdString();
    }
  }
}

// The examples of 0.8.0 - 0.8.5 stay recognisable: one task, note and
// meeting title per language at least, none of them the new content.
TEST(SampleData, LegacyExampleIsKept) {
  for(const SampleData::Lang lang : kLangs) {
    EXPECT_EQ(SampleData::legacy::tasks(lang).size(), 15);
    EXPECT_EQ(SampleData::legacy::notes(lang).size(), 5);
  }
  EXPECT_EQ(SampleData::legacy::eventTitles().size(), 10);
  EXPECT_TRUE(SampleData::legacy::eventTitles().contains(QStringLiteral("1:1 with Oleg")));
  const QVector<Task> legacyEn = SampleData::legacy::tasks(SampleData::Lang::En);  // findTask points into it
  const Task* old = findTask(legacyEn, QStringLiteral("APP-101"));
  ASSERT_NE(old, nullptr);
  EXPECT_EQ(old->title, QStringLiteral("Login rate-limit bypass"));
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
