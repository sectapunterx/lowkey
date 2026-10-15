import QtQuick
import TodoCpp

// The first tracker task opened with a description (coach mark): the text is
// the tracker's and a sync rewrites it; the user's own goes below, in My
// notes, which no sync touches. Step 1 sits by "From Jira · read-only",
// step 2 by "My notes".
CoachMark {
    id: root
    markId: "trackerText"
    stepCount: 2
    side: "left"

    // AppController.coachMark: {id, taskId, tracker}
    property var hint: ({})
    property Item headAnchor: null
    property Item notesAnchor: null
    readonly property string _tracker: String(root.hint.tracker || "")

    anchor: root.step === 1 || !root.notesAnchor ? root.headAnchor : root.notesAnchor
    title: root.step === 1 ? I18n.t("coach.trackerText.title").arg(root._tracker) : I18n.t("coach.trackerText.title2")
    body: I18n.t(root.step === 1 ? "coach.trackerText.body" : "coach.trackerText.body2")
    primaryText: I18n.t(root.step === 1 ? "coach.next" : "coach.gotIt")
    secondaryText: I18n.t(root.step === 1 ? "coach.skip" : "coach.back")
    onPrimaryClicked: {
        if (root.step === 1) root.step = 2;
        else root.finish();
    }
    onSecondaryClicked: {
        if (root.step === 1) root.finish();
        else root.step = 1;
    }
}
