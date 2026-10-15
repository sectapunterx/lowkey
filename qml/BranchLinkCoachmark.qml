pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import TodoCpp

// The first git switch, made while Tabless runs, to a branch with a task's
// key in its name (coach mark), under the "working on" line: why the task
// was picked, that the repository is only read, and where it is turned off.
//
//   NEW · 1 OF 2
//   The branch found its task
//   fix/ TEL-1 -jobs  →  TEL-1
//   Branch fix/TEL-1-jobs has TEL-1 in its name…
//   Esc close                  [Where to turn it off] [Got it]
CoachMark {
    id: root
    markId: "branchLink"
    stepCount: 2
    side: "below"

    // AppController.coachMark: {id, taskId, key, branch, moved}
    property var hint: ({})
    readonly property string _key: String(root.hint.key || "")
    readonly property string _branch: String(root.hint.branch || "")
    readonly property int _at: root._key.length > 0 ? root._branch.toUpperCase().indexOf(root._key.toUpperCase()) : -1

    title: I18n.t(root.step === 1 ? "coach.branchLink.title" : "coach.branchLink.title2")
    body: root.step === 2 ? I18n.t("coach.branchLink.body2")
        : I18n.t("coach.branchLink.body").arg(root._branch).arg(root._key)
          + (root.hint.moved === true ? " " + I18n.t("coach.branchLink.moved") : "")
    primaryText: I18n.t(root.step === 1 ? "coach.gotIt" : "coach.openSettings")
    secondaryText: I18n.t(root.step === 1 ? "coach.branchLink.how" : "coach.back")
    onSecondaryClicked: root.step = root.step === 1 ? 2 : 1
    onPrimaryClicked: {
        if (root.step === 2) root.openSettings("git", "settings-git-link");
        else root.finish();
    }

    // The key, found in the branch name.
    Rectangle {
        objectName: "coachmark-branch"
        visible: root.step === 1 && root._at >= 0
        Layout.fillWidth: true
        implicitHeight: branchRow.implicitHeight + 2 * Theme.spMd
        radius: Theme.radiusMd
        color: Theme.panel
        border.width: 1
        border.color: Theme.border
        RowLayout {
            id: branchRow
            anchors.fill: parent
            anchors.margins: Theme.spMd
            spacing: 0
            Mono { text: root._at > 0 ? root._branch.substring(0, root._at) : ""; color: Theme.textDim }
            Rectangle {
                implicitWidth: keyT.implicitWidth + 2 * Theme.spXs
                implicitHeight: keyT.implicitHeight
                radius: Theme.radiusSm
                color: Theme.accentSoft
                Mono { id: keyT; anchors.centerIn: parent; text: root._at >= 0 ? root._branch.substr(root._at, root._key.length) : "" }
            }
            Mono { text: root._at >= 0 ? root._branch.substring(root._at + root._key.length) : ""; color: Theme.textDim }
            Mono { text: "  →  "; color: Theme.textDim }
            Mono { text: root._key }
            Item { Layout.fillWidth: true }
        }
    }

    component Mono: Text {
        color: Theme.text
        font.family: Theme.fontMono
        font.pixelSize: Theme.fsXs
    }
}
