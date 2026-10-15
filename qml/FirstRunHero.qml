pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic as QQC
import TodoCpp

// The first run (APP-271): instead of a tour, an empty Today with the one
// input right on the page — one line, one task, the date, time and priority
// read from it the way Ctrl N reads them — the keys to know, and where tasks
// that already exist elsewhere come from. The example is a profile of its
// own; nothing here touches the person's tasks.
//
// Bold (H2-First): a heading, an emphasized box, three key cards, buttons.
// Quiet (Q-First, DG-111): a label, an underline input, two hint lines and
// three text links.
Item {
    id: root
    objectName: "first-run"

    signal created(string taskId)
    signal connectRequested()
    signal importRequested()
    signal exampleRequested()

    readonly property bool quiet: !Style.fills

    implicitHeight: col.implicitHeight

    function focusInput() { input.forceActiveFocus(); }
    // Hidden (the example opened, the first task made), the input lets go of
    // the keyboard: its FocusScope handed it straight back, and what was typed
    // went into an invisible field, Return making a hidden task (VERIFY-B-1).
    onVisibleChanged: if (!visible) input.focus = false

    // "Ctrl+K" as keymap.md writes it: "Ctrl K".
    function _keys(id, fallback) {
        const seq = AppController.shortcuts.length >= 0 ? AppController.shortcutFor(id) : "";
        const s = String(seq || fallback).split(", ")[0];
        return s.replace(/\+(?=.)/g, " ");
    }

    // The placeholder's lead word ("например:"), when the example was typed
    // out as shown: it is not part of the task (A5-9).
    function _withoutLead(raw) {
        for (const key of ["first.placeholder", "first.q.placeholder"]) {
            const ph = I18n.t(key);
            const cut = ph.indexOf(":");
            if (cut <= 0) continue;
            const lead = ph.slice(0, cut + 1).toLowerCase();
            if (raw.toLowerCase().startsWith(lead)) return raw.slice(lead.length).trim();
        }
        return raw;
    }

    // The same draft Ctrl N saves: title, "when", deadline, priority, labels.
    function submit() {
        const raw = root._withoutLead(input.text.trim());
        if (raw.length === 0) return "";
        const draft = AppController.quickTaskDraft(raw, new Date());
        if (String(draft.title).length === 0) return "";
        AppController.saveTask(draft);
        AppController.markWelcomeSeen();
        input.text = "";
        // The first task made for another day is shown on its day: Today
        // went empty, "Nothing planned", and the task was nowhere (A5-10).
        const on = draft.scheduledAt || draft.dueAt;
        if (on && on.getTime && !isNaN(on.getTime())) {
            const day = new Date(on.getFullYear(), on.getMonth(), on.getDate());
            if (Qt.formatDate(day, "yyyy-MM-dd") !== Qt.formatDate(AppController.today, "yyyy-MM-dd"))
                AppController.selectedDate = day;
        }
        root.created(draft.id);
        return draft.id;
    }

    ColumnLayout {
        id: col
        anchors.horizontalCenter: parent.horizontalCenter
        width: Math.min(parent.width, Theme.px(root.quiet ? 560 : 640))
        spacing: 0

        Text {
            objectName: "first-run-title"
            Layout.fillWidth: true
            text: I18n.t("first.title")
            color: root.quiet ? Theme.textMuted : Theme.text
            font.family: Theme.fontDisplay
            font.pixelSize: root.quiet ? Theme.fsLg : Theme.fsXl
            font.weight: root.quiet ? Theme.fwBody : Theme.fwHeading
            wrapMode: Text.WordWrap
            Accessible.role: Accessible.Heading
            Accessible.name: text
        }
        // The one thing to start with: the global capture key, read from
        // the keymap (Settings → Hotkeys can rebind or clear it).
        Text {
            objectName: "first-run-sub"
            readonly property string captureKey: AppController.shortcuts.length >= 0
                                                 ? AppController.shortcutText("quick-capture") : ""
            Layout.fillWidth: true
            Layout.topMargin: Theme.spSm
            text: captureKey.length > 0
                  ? I18n.t("first.sub").arg("<font face=\"" + Theme.fontMono + "\" color=\"" + Theme.text + "\">" + captureKey + "</font>")
                  : I18n.t("first.subNoKey")
            textFormat: Text.StyledText
            color: root.quiet ? Theme.textDim : Theme.textMuted
            font.family: Theme.fontUi
            font.pixelSize: Theme.fsMd
            wrapMode: Text.WordWrap
        }

        // The input, on the page: an emphasized box in bold, a line under
        // the words in quiet.
        Rectangle {
            Layout.fillWidth: true
            Layout.topMargin: root.quiet ? Theme.spLg : Theme.spXl
            implicitHeight: root.quiet ? Theme.px(44) : Theme.px(50)
            radius: root.quiet ? 0 : Theme.radiusXl
            color: root.quiet ? "transparent" : Theme.bg2
            border.width: root.quiet ? 0 : 1
            // Only real focus looks active: the brightest edge at rest read
            // as a caret that was not there (PERSONA-2).
            border.color: input.activeFocus ? Theme.focusRing : Theme.borderStrong
            Rectangle {
                visible: root.quiet
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: input.activeFocus ? Theme.focusRing : Theme.borderStrong
            }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: root.quiet ? 0 : Theme.spXl
                anchors.rightMargin: root.quiet ? 0 : Theme.spXl
                spacing: Theme.spMd
                QQC.TextField {
                    id: input
                    objectName: "first-run-input"
                    Layout.fillWidth: true
                    leftPadding: root.quiet ? Theme.spXs : 0
                    background: Item {}
                    color: Theme.text
                    font.family: Theme.fontUi
                    font.pixelSize: root.quiet ? Theme.fsXl : Theme.px(16)
                    placeholderText: I18n.t(root.quiet ? "first.q.placeholder" : "first.placeholder")
                    // Bold draws its own two-tone placeholder below (R4-007).
                    placeholderTextColor: root.quiet ? Theme.textDim : "transparent"
                    selectByMouse: true
                    Accessible.name: I18n.t("first.title")
                    onAccepted: root.submit()
                    // Esc leaves the field, so the page keys on the cards
                    // ("d", "?") work (A5-13).
                    Keys.onEscapePressed: (e) => {
                        e.accepted = true;
                        root.forceActiveFocus();
                    }
                    // H2-First: "например:" muted, the example brighter.
                    Text {
                        objectName: "first-run-placeholder"
                        visible: !root.quiet && input.text.length === 0 && !input.preeditText
                        anchors.left: parent.left
                        anchors.leftMargin: input.leftPadding
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        readonly property string _all: I18n.t("first.placeholder")
                        readonly property int _cut: _all.indexOf(":") + 1
                        text: "<font color=\"" + Theme.textDim + "\">" + _all.slice(0, _cut) + "</font>" + _all.slice(_cut)
                        textFormat: Text.StyledText
                        elide: Text.ElideRight
                        color: Theme.textMuted
                        font: input.font
                        Accessible.ignored: true
                    }
                }
                Text {
                    visible: !root.quiet
                    text: "Enter"
                    color: Theme.textDim
                    font.family: Theme.fontMono
                    font.pixelSize: Theme.fsXs
                    Accessible.ignored: true
                }
            }
        }

        // Quiet: two lines of hints instead of the cards.
        Text {
            visible: root.quiet
            Layout.fillWidth: true
            Layout.topMargin: Theme.spLg
            text: I18n.t("first.q.hintSave")
            color: Theme.textDim
            font.family: Theme.fontUi
            font.pixelSize: Theme.fsMd
            lineHeight: 1.5
            wrapMode: Text.WordWrap
        }
        // "? — все клавиши" (a-17, Q-First, DG-110): the cheat sheet's page
        // key, the one the bold cards name too, not Ctrl /.
        Text {
            objectName: "first-run-q-keys"
            visible: root.quiet
            Layout.fillWidth: true
            text: I18n.t("first.q.hintKeys")
                  .arg("<font face=\"" + Theme.fontMono + "\" color=\"" + Theme.textMuted + "\">" + root._keys("palette.open", "Ctrl+K") + "</font>")
                  .arg("<font face=\"" + Theme.fontMono + "\" color=\"" + Theme.textMuted + "\">" + root._keys("hotkeys.open.alt", "?") + "</font>")
            textFormat: Text.StyledText
            color: Theme.textDim
            font.family: Theme.fontUi
            font.pixelSize: Theme.fsMd
            lineHeight: 1.5
            wrapMode: Text.WordWrap
        }

        // Bold: three keys, always shown here: this is where they are learnt.
        // "d" and "?" are page keys: with the caret in the field above they
        // typed "d?" into it, and Enter made a task of that (A5-13). The line
        // says so, and Esc takes the caret out of the field.
        Text {
            objectName: "first-run-keys-where"
            visible: !root.quiet
            Layout.fillWidth: true
            Layout.topMargin: Theme.sp2xl + Theme.spXl
            text: I18n.t("first.keys.outside")
            color: Theme.textDim
            font.family: Theme.fontUi
            font.pixelSize: Theme.fsSm
            wrapMode: Text.WordWrap
        }
        RowLayout {
            visible: !root.quiet
            Layout.fillWidth: true
            // H2-First: 28px above and below the cards, 14px inside (R4-008).
            Layout.topMargin: Theme.spMd
            spacing: Theme.spLg
            Repeater {
                model: [
                    { keys: root._keys("palette.open", "Ctrl+K"), text: I18n.t("first.key.palette") },
                    { keys: root._keys("task.done", "D").toLowerCase(), text: I18n.t("first.key.done") },
                    { keys: root._keys("hotkeys.open.alt", "?"), text: I18n.t("first.key.keys") }
                ]
                delegate: Rectangle {
                    id: keyCard
                    required property var modelData
                    objectName: "first-run-key"
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredWidth: 1
                    implicitHeight: keyCol.implicitHeight + 2 * (Theme.spLg + Theme.spXs)
                    radius: Theme.radiusLg
                    color: Theme.surfaceCard
                    ColumnLayout {
                        id: keyCol
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: Theme.spLg + Theme.spXs
                        spacing: Theme.spSm
                        Text {
                            text: keyCard.modelData.keys
                            color: Theme.text
                            font.family: Theme.fontMono
                            font.pixelSize: Theme.fsMd
                        }
                        Text {
                            Layout.fillWidth: true
                            text: keyCard.modelData.text
                            color: Theme.textMuted
                            font.family: Theme.fontUi
                            font.pixelSize: Theme.fsSm
                            lineHeight: 1.2
                            wrapMode: Text.WordWrap
                        }
                    }
                }
            }
        }

        Rectangle {
            visible: !root.quiet
            Layout.fillWidth: true
            Layout.topMargin: Theme.sp2xl + Theme.spXl
            implicitHeight: 1
            color: Theme.border
        }

        // Tasks that already live somewhere else: buttons in bold, words in
        // quiet.
        Text {
            visible: root.quiet
            Layout.fillWidth: true
            Layout.topMargin: Theme.px(60)
            text: I18n.t("first.elsewhere")
            color: Theme.textDim
            font.family: Theme.fontUi
            font.pixelSize: Theme.fsMd
            wrapMode: Text.WordWrap
        }
        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: root.quiet ? Theme.spMd : Theme.spXl
            spacing: root.quiet ? Theme.spXl : Theme.spMd
            Text {
                visible: !root.quiet
                Layout.fillWidth: true
                text: I18n.t("first.elsewhere")
                color: Theme.textDim
                font.family: Theme.fontUi
                font.pixelSize: Theme.fsSm
                wrapMode: Text.WordWrap
            }
            PillButton {
                objectName: "first-run-connect"
                visible: !root.quiet
                text: I18n.t("first.connect")
                onClicked: root.connectRequested()
            }
            PillButton {
                objectName: "first-run-import"
                visible: !root.quiet
                text: I18n.t("first.import")
                onClicked: root.importRequested()
            }
            QuietLink { visible: root.quiet; text: I18n.t("first.q.connect"); onActivated: root.connectRequested() }
            QuietLink { visible: root.quiet; text: I18n.t("first.q.import"); onActivated: root.importRequested() }
            QuietLink { visible: root.quiet; text: I18n.t("first.q.example"); onActivated: root.exampleRequested() }
            Item { visible: root.quiet; Layout.fillWidth: true }
        }
        QuietLink {
            objectName: "first-run-example"
            visible: !root.quiet
            Layout.topMargin: Theme.spLg
            text: I18n.t("first.example")
            color: Theme.textMuted
            onActivated: root.exampleRequested()
        }
    }

    component QuietLink: Text {
        id: ql
        signal activated()
        color: Theme.textDim
        font.family: Theme.fontUi
        font.pixelSize: root.quiet ? Theme.fsMd : Theme.fsSm
        font.underline: true
        ClickArea {
            label: ql.text
            role: Accessible.Link
            onActivated: ql.activated()
        }
    }
}
