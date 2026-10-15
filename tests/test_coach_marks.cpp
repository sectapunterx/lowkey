// One-time coach marks (src/hints/CoachMarks.h): the seen-set, the rules for
// putting one on offer, and the controller's share — one waiting at a time,
// one per session, read once and never again. The triggers on tracker cards
// are in test_tracker_writes.cpp.

#include "AppController.h"
#include "Models.h"

#include "hints/CoachMarks.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QJsonObject>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>

#include <gtest/gtest.h>

#include <memory>

using namespace heap::hints;

TEST(CoachMarkRules, EveryIdIsKnownOnce) {
  QStringList ids = coachMarkIds();
  EXPECT_EQ(ids.size(), 9);
  EXPECT_EQ(ids.removeDuplicates(), 0);
  for(const char* id :
      {"reviewFold", "trackerMove", "trackerText", "branchLink", "captureDates", "conflict", "weekKeys", "timeMachine", "sprint"}) {
    EXPECT_TRUE(isCoachMark(QString::fromLatin1(id))) << id;
  }
  EXPECT_FALSE(isCoachMark(QStringLiteral("trackermove")));
  EXPECT_FALSE(isCoachMark(QString()));
}

TEST(CoachMarkRules, SeenIsOneKeyPerId) {
  QJsonObject settings;
  EXPECT_FALSE(coachMarkSeen(settings, QStringLiteral("sprint")));
  EXPECT_TRUE(markCoachMarkSeen(settings, QStringLiteral("sprint")));
  EXPECT_FALSE(markCoachMarkSeen(settings, QStringLiteral("sprint"))) << "nothing new to save";
  EXPECT_TRUE(coachMarkSeen(settings, QStringLiteral("sprint")));
  EXPECT_FALSE(coachMarkSeen(settings, QStringLiteral("conflict")));
  EXPECT_EQ(settings.value(QStringLiteral("hintsSeen")).toObject(), (QJsonObject{{QStringLiteral("sprint"), true}}));

  markCoachMarkSeen(settings, QStringLiteral("conflict"));
  forgetCoachMark(settings, QStringLiteral("sprint"));
  EXPECT_FALSE(coachMarkSeen(settings, QStringLiteral("sprint")));
  EXPECT_TRUE(coachMarkSeen(settings, QStringLiteral("conflict")));
  forgetCoachMark(settings);
  EXPECT_FALSE(settings.contains(QStringLiteral("hintsSeen")));
}

// 0.9.4's first coach mark kept a flag of its own: still read, and folded
// into the set the next time anything is marked.
TEST(CoachMarkRules, TheOldReviewFoldFlagCountsAsSeen) {
  QJsonObject settings{{QStringLiteral("reviewFoldHintSeen"), true}};
  EXPECT_TRUE(coachMarkSeen(settings, QStringLiteral("reviewFold")));
  EXPECT_FALSE(mayOfferCoachMark(settings, QStringLiteral("reviewFold"), false, false));
  EXPECT_TRUE(markCoachMarkSeen(settings, QStringLiteral("weekKeys")));
  EXPECT_FALSE(settings.contains(QStringLiteral("reviewFoldHintSeen")));
  EXPECT_TRUE(coachMarkSeen(settings, QStringLiteral("reviewFold")));
  EXPECT_TRUE(coachMarkSeen(settings, QStringLiteral("weekKeys")));
}

TEST(CoachMarkRules, OfferNeedsAnUnreadKnownIdAndAFreeSession) {
  const QJsonObject none;
  EXPECT_TRUE(mayOfferCoachMark(none, QStringLiteral("sprint"), false, false));
  EXPECT_FALSE(mayOfferCoachMark(none, QStringLiteral("nope"), false, false));
  EXPECT_FALSE(mayOfferCoachMark(none, QStringLiteral("sprint"), /*oneWaiting=*/true, false));
  EXPECT_FALSE(mayOfferCoachMark(none, QStringLiteral("sprint"), false, /*shownThisSession=*/true));
}

class CoachMarks : public ::testing::Test {
 protected:
  void SetUp() override {
    app_ = std::make_unique<AppController>();
    app_->tasks()->reset({});
    app_->setCoachMarksEnabled(true);
    app_->forgetCoachMarks();
  }

  void TearDown() override {
    app_->forgetCoachMarks();
    app_.reset();
  }

  QString markId() const {
    return app_->coachMark().value(QStringLiteral("id")).toString();
  }

  std::unique_ptr<AppController> app_;
};

TEST_F(CoachMarks, OneWaitsAtATime_AndAnUnknownIdIsRefused) {
  QSignalSpy changed(app_.get(), &AppController::coachMarkChanged);
  EXPECT_FALSE(app_->offerCoachMark(QStringLiteral("notAHint")));
  EXPECT_TRUE(app_->coachMark().isEmpty());
  EXPECT_TRUE(app_->offerCoachMark(QStringLiteral("sprint"), {{QStringLiteral("name"), QStringLiteral("S 14")}}));
  EXPECT_EQ(markId(), QStringLiteral("sprint"));
  EXPECT_EQ(app_->coachMark().value(QStringLiteral("name")).toString(), QStringLiteral("S 14"));
  EXPECT_FALSE(app_->offerCoachMark(QStringLiteral("weekKeys"))) << "two would be waiting";
  EXPECT_EQ(markId(), QStringLiteral("sprint"));
  EXPECT_EQ(changed.count(), 1);
}

// Shown and read: never again, and nothing else this session.
TEST_F(CoachMarks, ReadOnce_AndOnePerSession) {
  ASSERT_TRUE(app_->offerCoachMark(QStringLiteral("sprint")));
  app_->coachMarkShown(QStringLiteral("sprint"));
  EXPECT_TRUE(app_->coachMark().value(QStringLiteral("shown")).toBool());
  app_->dismissCoachMark(QStringLiteral("sprint"));
  EXPECT_TRUE(app_->coachMark().isEmpty());
  EXPECT_TRUE(app_->coachMarkSeen(QStringLiteral("sprint")));
  EXPECT_FALSE(app_->offerCoachMark(QStringLiteral("weekKeys"))) << "a second coach mark in one session";

  // The next session: another one may come, the read one never.
  app_->flushSave();
  app_ = std::make_unique<AppController>();
  app_->tasks()->reset({});
  app_->setCoachMarksEnabled(true);
  EXPECT_FALSE(app_->offerCoachMark(QStringLiteral("sprint")));
}

// An anchor that went away before showing hands it back unread: another
// may come, and this one again later.
TEST_F(CoachMarks, DroppedUnshownStaysUnread) {
  ASSERT_TRUE(app_->offerCoachMark(QStringLiteral("trackerText")));
  app_->dropCoachMark(QStringLiteral("trackerText"));
  EXPECT_TRUE(app_->coachMark().isEmpty());
  EXPECT_FALSE(app_->coachMarkSeen(QStringLiteral("trackerText")));
  EXPECT_TRUE(app_->offerCoachMark(QStringLiteral("trackerText")));
  app_->dropCoachMark(QStringLiteral("weekKeys"));
  EXPECT_EQ(markId(), QStringLiteral("trackerText")) << "dropping another id leaves this one";
}

// Read from the settings it points to, without ever being shown: still read.
TEST_F(CoachMarks, DismissWithoutShowingStillReads) {
  app_->dismissCoachMark(QStringLiteral("reviewFold"));
  EXPECT_TRUE(app_->coachMarkSeen(QStringLiteral("reviewFold")));
  EXPECT_FALSE(app_->offerCoachMark(QStringLiteral("reviewFold")));
  EXPECT_TRUE(app_->offerCoachMark(QStringLiteral("sprint"))) << "nothing was on screen this session";
}

// Under test mode they are off unless a test turns them on: a coach mark
// must not take the keyboard in another suite's flow.
TEST_F(CoachMarks, OffInTestModeUntilTurnedOn) {
  AppController fresh;
  EXPECT_FALSE(fresh.offerCoachMark(QStringLiteral("sprint")));
  fresh.setCoachMarksEnabled(true);
  EXPECT_TRUE(fresh.offerCoachMark(QStringLiteral("sprint")));
  fresh.forgetCoachMarks();
}

TEST_F(CoachMarks, UsesAreCountedPerIdThisSession) {
  EXPECT_EQ(app_->noteCoachMarkUse(QStringLiteral("weekKeys")), 1);
  EXPECT_EQ(app_->noteCoachMarkUse(QStringLiteral("weekKeys")), 2);
  EXPECT_EQ(app_->noteCoachMarkUse(QStringLiteral("sprint")), 1);
  EXPECT_EQ(app_->noteCoachMarkUse(QStringLiteral("weekKeys")), 3);
}

// Five or more deleted at once: the Time Machine coach mark. Fewer: none.
TEST_F(CoachMarks, TimeMachine_OfferedOnABulkDeleteOfFive) {
  QVector<Task> tasks;
  for(int i = 0; i < 9; ++i) {
    Task t;
    t.id = QStringLiteral("t%1").arg(i);
    t.title = QStringLiteral("task %1").arg(i);
    t.status = QStringLiteral("todo");
    tasks.append(t);
  }
  app_->tasks()->reset(tasks);
  app_->setSelectedTaskIds({QStringLiteral("t0"), QStringLiteral("t1"), QStringLiteral("t2"), QStringLiteral("t3")});
  app_->deleteSelectedTasks();
  EXPECT_TRUE(app_->coachMark().isEmpty()) << "four is not a bulk delete";

  app_->setSelectedTaskIds({QStringLiteral("t4"), QStringLiteral("t5"), QStringLiteral("t6"), QStringLiteral("t7"), QStringLiteral("t8")});
  app_->deleteSelectedTasks();
  EXPECT_EQ(markId(), QStringLiteral("timeMachine"));
  EXPECT_EQ(app_->coachMark().value(QStringLiteral("count")).toInt(), 5);
}

int main(int argc, char** argv) {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QStandardPaths::setTestModeEnabled(true);
  QTemporaryDir scratch;
  scratch.setAutoRemove(true);
  qputenv("XDG_CONFIG_HOME", scratch.path().toUtf8());
  qputenv("XDG_DATA_HOME", scratch.path().toUtf8());

  QApplication qapp(argc, argv);

  const QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  if(!appData.isEmpty()) {
    QFile::remove(appData + QStringLiteral("/state.json"));
    QDir(appData + QStringLiteral("/backups")).removeRecursively();
  }

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
