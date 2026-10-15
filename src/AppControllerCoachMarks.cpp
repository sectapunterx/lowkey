// One-time coach marks (src/hints/CoachMarks.h): which one waits for its
// anchor, and which were read. The rules live in the header; this is the
// controller's share — the one on offer, the session's one, the save.

#include "AppController.h"

#include "hints/CoachMarks.h"

#include <QDebug>

bool AppController::offerCoachMark(const QString& id, const QVariantMap& payload) {
  if(!heap::hints::isCoachMark(id)) {
    qWarning() << "offerCoachMark: unknown coach mark" << id;
    return false;
  }
  if(!m_coachMarksEnabled) {
    return false;
  }
  if(!heap::hints::mayOfferCoachMark(m_settingsExtra, id, !m_coachMark.isEmpty(), m_coachMarkShownThisSession)) {
    return false;
  }
  m_coachMark = payload;
  m_coachMark.insert(QStringLiteral("id"), id);
  emit coachMarkChanged();
  return true;
}

void AppController::coachMarkShown(const QString& id) {
  if(m_coachMark.value(QStringLiteral("id")).toString() == id && !m_coachMark.value(QStringLiteral("shown")).toBool()) {
    m_coachMarkShownThisSession = true;
    // On screen once: a second anchor for the same task (the card in two
    // views) leaves it be.
    m_coachMark.insert(QStringLiteral("shown"), true);
    emit coachMarkChanged();
  }
}

void AppController::dismissCoachMark(const QString& id) {
  if(!heap::hints::isCoachMark(id)) {
    return;
  }
  if(heap::hints::markCoachMarkSeen(m_settingsExtra, id)) {
    scheduleSave();
  }
  if(m_coachMark.value(QStringLiteral("id")).toString() == id) {
    m_coachMarkShownThisSession = true;
    m_coachMark.clear();
    emit coachMarkChanged();
  }
}

void AppController::dropCoachMark(const QString& id) {
  if(!m_coachMark.isEmpty() && m_coachMark.value(QStringLiteral("id")).toString() == id) {
    m_coachMark.clear();
    emit coachMarkChanged();
  }
}

bool AppController::coachMarkSeen(const QString& id) const {
  return heap::hints::coachMarkSeen(m_settingsExtra, id);
}

void AppController::setCoachMarksEnabled(bool on) {
  m_coachMarksEnabled = on;
}

int AppController::noteCoachMarkUse(const QString& id) {
  return ++m_coachMarkUses[id];
}

void AppController::forgetCoachMarks(const QString& id) {
  heap::hints::forgetCoachMark(m_settingsExtra, id);
  m_coachMarkShownThisSession = false;
  m_coachMarkUses.clear();
  if(!m_coachMark.isEmpty()) {
    m_coachMark.clear();
    emit coachMarkChanged();
  }
}
