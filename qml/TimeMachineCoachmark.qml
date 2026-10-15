import QtQuick
import TodoCpp

// The first bulk delete of five tasks or more (coach mark), over its
// "Tasks deleted" toast: Undo there now, and the Time Machine for later.
// It takes no keys and covers no part of the toast, so Undo works as ever.
//
//   NEW
//   Deleted is not gone
//   Right now: Undo in the notice (Ctrl+Z). Later: the Time Machine…
//   Esc close                              [Time Machine] [Got it]
CoachMark {
    id: root
    markId: "timeMachine"
    side: "above"
    centered: true
    takesFocus: false

    signal openTimeMachine()

    title: I18n.t("coach.timeMachine.title")
    body: I18n.t("coach.timeMachine.body").arg(AppController.shortcutText("undo"))
    secondaryText: I18n.t("coach.timeMachine.open")
    onPrimaryClicked: root.finish()
    onSecondaryClicked: root.finish(function () { root.openTimeMachine(); })
}
