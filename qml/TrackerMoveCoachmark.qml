import QtQuick
import TodoCpp

// The first tracker card moved while writing to that tracker is off (coach
// mark): the move stayed here, and where writing is turned on. It turns
// nothing on itself.
//
//   NEW · 1 OF 2
//   Moved only in Tabless
//   In Jira the ticket is still in “To Do”. Tabless writes nothing…
//   Esc close                       [How to send it] [Got it]
CoachMark {
    id: root
    markId: "trackerMove"
    stepCount: 2

    // AppController.coachMark: {id, taskId, provider, tracker, remoteStatus}
    property var hint: ({})
    readonly property string _tracker: String(root.hint.tracker || root.hint.provider || "")
    readonly property string _remote: String(root.hint.remoteStatus || "")

    title: root.step === 1 ? I18n.t("coach.trackerMove.title") : I18n.t("coach.trackerMove.title2").arg(root._tracker)
    body: root.step === 2 ? I18n.t("coach.trackerMove.body2").arg(root._tracker)
        : root._remote.length > 0 ? I18n.t("coach.trackerMove.body").arg(root._tracker).arg(root._remote)
        : I18n.t("coach.trackerMove.bodyNoStatus").arg(root._tracker)
    primaryText: I18n.t(root.step === 1 ? "coach.gotIt" : "coach.openSettings")
    secondaryText: I18n.t(root.step === 1 ? "coach.trackerMove.how" : "coach.back")
    onSecondaryClicked: root.step = root.step === 1 ? 2 : 1
    onPrimaryClicked: {
        if (root.step === 2)
            root.openSettings("integrations", "int-write-status-" + String(root.hint.provider || ""));
        else
            root.finish();
    }
}
