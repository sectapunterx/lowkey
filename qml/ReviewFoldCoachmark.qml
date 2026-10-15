pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import TodoCpp

// The one-time coach mark beside the first card a review was folded into
// (0.9.4, after APP-293): the merge request is shown on the ticket's card, not
// as a card of its own, and here is where that is turned off.
//
//   NEW · 1 OF 2
//   The request and the ticket are one card
//   [Jira · TEL-1]  ⟶  [TEL-1 · MR !17]
//   MR !17 from GitLab closes TEL-1, so it shows here…
//   Esc close                    [How to turn it off] [Got it]
//
// Step 2 says where the switch is and opens Settings on it. Any way out
// (Got it, Esc, a press outside, the settings) counts as read: it never
// comes back.
CoachMark {
    id: root
    markId: "reviewFold"
    stepCount: 2

    // AppController.coachMark: {id, taskId, issueKey, provider, providerName, mr}
    property var hint: ({})
    readonly property string _mr: String(root.hint.mr || "")
    readonly property string _key: String(root.hint.issueKey || "")
    readonly property string _from: String(root.hint.providerName || root.hint.provider || "")

    title: I18n.t(root.step === 1 ? "foldhint.title" : "foldhint.title2")
    body: root.step === 1 ? I18n.t("foldhint.body").arg((root.hint.provider === "github" ? "PR " : "MR ") + root._mr).arg(root._from).arg(root._key)
                          : I18n.t("foldhint.body2").arg(root._from)
    primaryText: I18n.t(root.step === 1 ? "coach.gotIt" : "coach.openSettings")
    secondaryText: I18n.t(root.step === 1 ? "foldhint.how" : "coach.back")
    onSecondaryClicked: root.step = root.step === 1 ? 2 : 1
    onPrimaryClicked: {
        if (root.step === 2)
            root.openSettings("integrations", "int-review-fold-" + String(root.hint.provider || ""));
        else
            root.finish();
    }

    // Two sources, one card: what the fold did, drawn once.
    Rectangle {
        visible: root.step === 1
        Layout.fillWidth: true
        implicitHeight: pair.implicitHeight + 2 * Theme.spMd
        radius: Theme.radiusMd
        color: Theme.panel
        border.width: 1
        border.color: Theme.border
        RowLayout {
            id: pair
            anchors.fill: parent
            anchors.margins: Theme.spMd
            spacing: Theme.spMd
            ColumnLayout {
                spacing: Theme.spXs
                FoldChip { text: "Jira · " + root._key }
                FoldChip { text: root._from + " · " + root._mr; ink: Theme.info }
            }
            Text {
                text: "→"
                color: Theme.textDim
                font.family: Theme.fontUi
                font.pixelSize: Theme.fsLg
            }
            FoldChip {
                text: root._key + " · " + root._mr
                edge: Theme.accent
            }
            Item { Layout.fillWidth: true }
        }
    }

    component FoldChip: Rectangle {
        id: chip
        property alias text: chipT.text
        property color ink: Theme.textMuted
        property color edge: Theme.borderStrong
        implicitWidth: chipT.implicitWidth + 2 * Theme.spSm
        implicitHeight: chipT.implicitHeight + Theme.spXs * 2
        radius: Theme.radiusSm
        color: "transparent"
        border.width: 1
        border.color: chip.edge
        Text {
            id: chipT
            anchors.centerIn: parent
            color: chip.ink
            font.family: Theme.fontUi
            font.pixelSize: Theme.fsXs
        }
    }
}
