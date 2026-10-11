#pragma once

#include "Models.h"

#include <QStringList>

// The example profile (APP-271): a made-up workday on a small web team, for
// someone who has just installed Tabless and has no tracker connected. Every
// task in it is a plain local task. The keys only look like a tracker's
// (APP-112, web#431): no task carries an externalProvider, so no sync, push
// or tracker prompt can ever reach one of them.
namespace SampleData {
// Seed-content language. Selects between the EN and RU example. The default
// mirrors AppController's default language ("en").
enum class Lang { En, Ru };

// Dates are relative to the day the example is opened, so it never looks
// stale: today's plan, one overdue task, deadlines today and tomorrow.
QVector<Task> tasks(Lang lang = Lang::En);
QVector<CalEvent> events(const QDate& today, Lang lang = Lang::En);
QVector<Person> people(Lang lang = Lang::En);
// Column names follow the seed language; the ids never change, so a column
// renamed or translated still means the same thing to sync and automation.
QVector<QVariantMap> statuses(Lang lang = Lang::En);
// The guide first (pinned, open), then the notes a developer would keep.
QVector<Note> notes(Lang lang = Lang::En);
// Who the example's tasks wait on. Task and person ids are the same in both
// languages.
QVector<WaitingOn> waitingOn();
// The quick-capture lines the guide note shows, one per line, so a test can
// check that each one reads the way the note says it does.
QStringList captureExamples(Lang lang = Lang::En);

// What the example held before it was reworked (0.8.0 – 0.8.5). An example
// profile made by one of those versions is still in people's state.json; its
// samples must still read as samples, so removing it takes its meetings and
// the question before removing does not count them as the person's work.
// Never seeded again. Only the fields the comparison reads are meaningful.
namespace legacy {
QVector<Task> tasks(Lang lang);
QStringList eventTitles();
QVector<Note> notes(Lang lang);
}  // namespace legacy
}  // namespace SampleData
