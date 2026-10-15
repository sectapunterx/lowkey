pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
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
Popup {
    PopupArrival { popup: root; dialog: false }
    id: root
    objectName: "review-fold-coachmark"
    width: Theme.px(400)
    padding: Theme.spXl
    background: PopupSurface {}
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    // AppController.reviewFoldHint: {taskId, issueKey, provider, providerName, mr}
    property var hint: ({})
    property int step: 1
    readonly property string _mr: String(root.hint.mr || "")
    readonly property string _key: String(root.hint.issueKey || "")
    readonly property string _from: String(root.hint.providerName || root.hint.provider || "")

    // Settings, opened on the switch this step names (APP-210's reveal).
    function _openSettings(provider) {
        // Main.qml's own function: the window type does not declare it.
        const w = root.ApplicationWindow.window;
        const reveal = w ? Reflect.get(w, "openSettingsItem") : null;
        if (typeof reveal === "function")
            reveal({ section: "integrations", objectName: "int-review-fold-" + provider });
    }

    onClosed: AppController.dismissReviewFoldHint()

    contentItem: ColumnLayout {
        spacing: Theme.spLg

        Text {
            objectName: "review-fold-badge"
            text: I18n.t("foldhint.badge").arg(root.step).toUpperCase()
            color: Theme.accentStrong
            font.family: Theme.fontUi
            font.pixelSize: Theme.fsXs
            font.weight: Theme.fwTitle
        }
        Text {
            objectName: "review-fold-title"
            Layout.fillWidth: true
            text: I18n.t(root.step === 1 ? "foldhint.title" : "foldhint.title2")
            wrapMode: Text.WordWrap
            color: Theme.text
            font.family: Theme.fontUi
            font.pixelSize: Theme.fsLg
            font.weight: Theme.fwTitle
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

        Text {
            objectName: "review-fold-body"
            Layout.fillWidth: true
            text: root.step === 1 ? I18n.t("foldhint.body").arg((root.hint.provider === "github" ? "PR " : "MR ") + root._mr).arg(root._from).arg(root._key)
                                  : I18n.t("foldhint.body2").arg(root._from)
            textFormat: Text.PlainText
            wrapMode: Text.WordWrap
            color: Theme.textMuted
            font.family: Theme.fontUi
            font.pixelSize: Theme.fsSm
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: Theme.spXs
            spacing: Theme.spSm
            KeyHint { keys: "Esc"; always: true }
            Text {
                text: I18n.t("foldhint.close")
                color: Theme.textDim
                font.family: Theme.fontUi
                font.pixelSize: Theme.fsXs
            }
            Item { Layout.fillWidth: true }
            PillButton {
                objectName: "review-fold-secondary"
                small: true
                text: I18n.t(root.step === 1 ? "foldhint.how" : "foldhint.back")
                onClicked: root.step = root.step === 1 ? 2 : 1
            }
            PillButton {
                objectName: "review-fold-primary"
                small: true
                primary: true
                text: I18n.t(root.step === 1 ? "foldhint.gotIt" : "foldhint.openSettings")
                onClicked: {
                    const provider = String(root.hint.provider || "");
                    const toSettings = root.step === 2;
                    root.close();
                    if (toSettings) root._openSettings(provider);
                }
            }
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
