import QtQuick
import TodoCpp

// The first card changed both here and in its tracker (coach mark), beside
// its "changed here and in Jira" mark: the local value is kept, nothing was
// sent, and the choice is the user's. The card does not choose.
//
//   NEW
//   Changed here and in Jira
//   Your value is kept, and nothing went to Jira…
//   Esc close                         [Later] [Compare and choose]
CoachMark {
    id: root
    markId: "conflict"

    // AppController.coachMark: {id, taskId, field, tracker}
    property var hint: ({})
    readonly property string _tracker: String(root.hint.tracker || "")
    // The compare dialog, opened by the card.
    signal compare()

    title: I18n.t("coach.conflict.title").arg(root._tracker)
    body: I18n.t("coach.conflict.body").arg(root._tracker)
    primaryText: I18n.t("coach.conflict.compare")
    secondaryText: I18n.t("coach.later")
    onSecondaryClicked: root.finish()
    onPrimaryClicked: root.finish(function () { root.compare(); })
}
