#pragma once

#include <QJsonObject>
#include <QString>
#include <QStringList>

// One-time coach marks (0.9.4+): a card beside the thing a feature just did
// on its own, saying what happened and where its switch is. Each one fires at
// the moment its feature first acts — never on startup — once per install,
// at most one on screen and one per session. A coach mark explains and points
// to Settings; it never turns anything on.
//
// Which ones were read lives in state.json under settings.hintsSeen, one key
// per id ({"trackerMove": true, …}), so "once" survives a restart.
namespace heap::hints {

// Every coach mark heap knows. A trigger with an id not listed here is a bug
// and is refused, so a typo cannot show a card that can never be marked read.
inline const QStringList& coachMarkIds() {
  static const QStringList ids{
      QStringLiteral("reviewFold"),    // a request folded into its Jira ticket
      QStringLiteral("trackerMove"),   // a tracker card moved only in Tabless
      QStringLiteral("trackerText"),   // the tracker's text vs "My notes"
      QStringLiteral("branchLink"),    // a git branch found its task
      QStringLiteral("captureDates"),  // dates read out of quick capture
      QStringLiteral("conflict"),      // a field changed here and in the tracker
      QStringLiteral("weekKeys"),      // keys for the week arrows
      QStringLiteral("timeMachine"),   // a bulk delete, and where it can come back from
      QStringLiteral("sprint"),        // a Jira sprint end in the week
  };
  return ids;
}

// A bulk delete of this many tasks or more tells about the Time Machine.
inline constexpr int kBulkDeleteHintAt = 5;

inline bool isCoachMark(const QString& id) {
  return coachMarkIds().contains(id);
}

inline QString seenKey() {
  return QStringLiteral("hintsSeen");
}

// 0.9.4's first coach mark kept its own flag; read as "reviewFold" seen.
inline QString legacyReviewFoldKey() {
  return QStringLiteral("reviewFoldHintSeen");
}

// `settings` is the settings object the seen-set lives in.
inline bool coachMarkSeen(const QJsonObject& settings, const QString& id) {
  if(settings.value(seenKey()).toObject().value(id).toBool()) {
    return true;
  }
  return id == QStringLiteral("reviewFold") && settings.value(legacyReviewFoldKey()).toBool();
}

// Marks `id` read. True when that changed anything (and wants saving).
inline bool markCoachMarkSeen(QJsonObject& settings, const QString& id) {
  bool changed = false;
  QJsonObject seen = settings.value(seenKey()).toObject();
  if(settings.contains(legacyReviewFoldKey())) {
    if(settings.value(legacyReviewFoldKey()).toBool()) {
      seen.insert(QStringLiteral("reviewFold"), true);
    }
    settings.remove(legacyReviewFoldKey());
    changed = true;
  }
  if(!seen.value(id).toBool()) {
    seen.insert(id, true);
    changed = true;
  }
  if(changed) {
    settings.insert(seenKey(), seen);
  }
  return changed;
}

// Unread again: `id`, or every coach mark when `id` is empty.
inline void forgetCoachMark(QJsonObject& settings, const QString& id = {}) {
  if(id.isEmpty() || id == QStringLiteral("reviewFold")) {
    settings.remove(legacyReviewFoldKey());
  }
  if(id.isEmpty()) {
    settings.remove(seenKey());
    return;
  }
  QJsonObject seen = settings.value(seenKey()).toObject();
  seen.remove(id);
  if(seen.isEmpty()) {
    settings.remove(seenKey());
  } else {
    settings.insert(seenKey(), seen);
  }
}

// Whether `id` may go on offer now: a known one, not read yet, nothing else
// waiting for its anchor, and no coach mark shown yet this session.
inline bool mayOfferCoachMark(const QJsonObject& settings, const QString& id, bool oneWaiting, bool shownThisSession) {
  return isCoachMark(id) && !oneWaiting && !shownThisSession && !coachMarkSeen(settings, id);
}

}  // namespace heap::hints
