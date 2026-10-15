pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import TodoCpp

// The one-time coach mark card every hint is drawn with (src/hints/CoachMarks.h):
//
//   NEW · 1 OF 2
//   Title
//   [what it did, drawn — optional]
//   Body
//   Esc close                         [secondary] [primary]
//
// Beside its anchor, never over a modal and never while typing (CoachMarkHost
// decides when). Any way out — a button, Esc, a press outside — counts as
// read: it never comes back. It explains and points to Settings; it never
// turns anything on. The primary button is the neutral one (DG-005).
Popup {
    PopupArrival { popup: root; dialog: false }
    id: root

    // The id in src/hints/CoachMarks.h, and the item it is about.
    required property string markId
    property Item anchor: null
    // "right" (flips left when there is no room), "left", "below", "above".
    property string side: "right"
    // Below or above: centred on the anchor rather than at its left edge.
    property bool centered: false
    property int step: 1
    property int stepCount: 1
    // "New" / "New · 1 of 2" unless a coach mark says otherwise.
    property string badgeText: root.stepCount > 1 ? I18n.t("coach.badge").arg(root.step).arg(root.stepCount) : I18n.t("coach.new")
    property string title: ""
    property string body: ""
    property string primaryText: I18n.t("coach.gotIt")
    property string secondaryText: ""
    // Steals no keys: quick capture keeps its caret (captureDates).
    property bool takesFocus: true
    // What the coach mark points at, drawn between title and body.
    default property alias extra: extraBox.data

    signal primaryClicked()
    signal secondaryClicked()

    objectName: "coachmark-" + root.markId
    parent: root.anchor
    width: Theme.px(400)
    padding: Theme.spXl
    margins: Theme.spMd
    focus: root.takesFocus
    background: PopupSurface {}
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    // Runs `after` (a dialog the card opens), then closes: closing reads the
    // coach mark, and its anchor lets go of it straight after.
    function finish(after) {
        if (after) after();
        root.close();
    }
    // Settings, opened on the row a coach mark names (APP-210's reveal).
    // Main.qml's window (openSettingsItem, showQuery…), or none in a test.
    readonly property var hostWindow: root.anchor ? root.anchor.Window.window : null
    function openSettings(section, objectName) {
        const w = root.hostWindow;
        root.close();
        if (w && w.openSettingsItem)
            w.openSettingsItem({ section: section, objectName: objectName || "" });
    }

    function _place() {
        const a = root.anchor;
        if (!a) return;
        const gap = Theme.spMd;
        let s = root.side;
        if (s === "right") {
            const p = a.mapToItem(null, a.width, 0);
            if (a.Window.width > 0 && p.x + gap + root.width > a.Window.width - Theme.spMd) s = "left";
        }
        if (s === "right") { root.x = a.width + gap; root.y = 0; }
        else if (s === "left") { root.x = -root.width - gap; root.y = 0; }
        else if (s === "below") { root.x = root.centered ? (a.width - root.width) / 2 : 0; root.y = a.height + Theme.spSm; }
        else { root.x = root.centered ? (a.width - root.width) / 2 : 0; root.y = -root.height - Theme.spSm; }
    }

    onAboutToShow: root._place()
    onStepChanged: if (root.opened) Qt.callLater(root._place)
    onAnchorChanged: if (root.opened) Qt.callLater(root._place)

    onOpened: AppController.coachMarkShown(root.markId)
    onClosed: AppController.dismissCoachMark(root.markId)

    contentItem: ColumnLayout {
        spacing: Theme.spLg

        // Without the keyboard (takesFocus false) Esc still reads it — the
        // first Esc closes the card, the next one is the anchor's again. In
        // an item: a Shortcut finds its window through one.
        Shortcut {
            sequence: "Esc"
            enabled: root.opened && !root.takesFocus
            onActivated: root.close()
        }

        Text {
            objectName: "coachmark-badge"
            text: root.badgeText.toUpperCase()
            color: Theme.accentStrong
            font.family: Theme.fontUi
            font.pixelSize: Theme.fsXs
            font.weight: Theme.fwTitle
        }
        Text {
            objectName: "coachmark-title"
            Layout.fillWidth: true
            text: root.title
            wrapMode: Text.WordWrap
            color: Theme.text
            font.family: Theme.fontUi
            font.pixelSize: Theme.fsLg
            font.weight: Theme.fwTitle
        }
        ColumnLayout {
            id: extraBox
            Layout.fillWidth: true
            visible: extraBox.children.length > 0
            spacing: Theme.spSm
        }
        Text {
            objectName: "coachmark-body"
            Layout.fillWidth: true
            visible: root.body.length > 0
            text: root.body
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
                text: I18n.t("coach.close")
                color: Theme.textDim
                font.family: Theme.fontUi
                font.pixelSize: Theme.fsXs
            }
            Item { Layout.fillWidth: true }
            PillButton {
                objectName: "coachmark-secondary"
                visible: root.secondaryText.length > 0
                small: true
                text: root.secondaryText
                onClicked: root.secondaryClicked()
            }
            PillButton {
                objectName: "coachmark-primary"
                small: true
                primary: true
                text: root.primaryText
                onClicked: root.primaryClicked()
            }
        }
    }
}
