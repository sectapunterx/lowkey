pragma ComponentBehavior: Bound
// "Перенести в профиль" (A5-14, sheet X/N-Dlg-Small): the task or the
// selection, the other profiles under a field that filters them; the arrows
// walk the list, Enter moves to the highlighted one, Esc leaves it. One undo
// step, in this profile and in the one it went to.
import QtQuick
import QtQuick.Layouts
import TodoCpp

SmallDialog {
    id: root
    objectName: "profile-move-dialog"

    property var taskIds: []
    property string query: ""
    property int current: 0
    // Tracker cards among them stay where they are; said before Enter.
    property var _trackerKeys: []

    readonly property var choices: {
        const q = root.query.trim().toLowerCase();
        const all = AppController.profiles || [];
        const out = [];
        for (let i = 0; i < all.length; i++) {
            const p = all[i];
            if (p.id === AppController.activeProfileId) continue;
            if (q.length > 0 && String(p.name || "").toLowerCase().indexOf(q) < 0) continue;
            out.push(p);
        }
        // A name that starts with what was typed first.
        if (q.length > 0)
            out.sort((a, b) => (String(a.name).toLowerCase().indexOf(q) === 0 ? 0 : 1)
                             - (String(b.name).toLowerCase().indexOf(q) === 0 ? 0 : 1));
        return out;
    }
    onChoicesChanged: if (root.current >= root.choices.length) root.current = Math.max(0, root.choices.length - 1)

    title: I18n.t("moveProfile.title")
    fact: {
        const ids = root.taskIds || [];
        if (ids.length === 1) {
            const t = AppController.taskById(ids[0]);
            return t && t.id ? t.id + " · " + (t.title || "") : ids[0];
        }
        return I18n.count(ids.length, "moveProfile.count");
    }
    factColor: (root.taskIds || []).length === 1 ? Theme.textMuted : Theme.textDim

    function openFor(ids) {
        root.taskIds = (ids || []).slice();
        const kept = [];
        for (const id of root.taskIds) {
            const t = AppController.taskById(id);
            if (t && t.ticket && t.ticket.provider) kept.push(t.ticket.key || t.id);
        }
        root._trackerKeys = kept;
        // Only tracker cards: nothing can move, so no picker with a Move
        // button that cannot happen (F1-new-6). The controller's refusal says
        // why at once — it moves nothing when every id is a tracker card.
        if (kept.length > 0 && kept.length === root.taskIds.length) {
            const all = AppController.profiles || [];
            for (let i = 0; i < all.length; i++) {
                if (all[i].id === AppController.activeProfileId) continue;
                AppController.moveTasksToProfile(root.taskIds, all[i].id);
                break;
            }
            return;
        }
        root.query = "";
        root.current = 0;
        filterField.text = "";
        open();
        filterField.focusField();
    }

    function move(delta) {
        if (root.choices.length === 0) return;
        root.current = (root.current + delta + root.choices.length) % root.choices.length;
    }

    function submit() {
        const p = root.choices[root.current];
        if (!p) return;
        const ids = root.taskIds;
        root.close();
        AppController.moveTasksToProfile(ids, p.id);
    }
    onAccepted: root.submit()

    DialogField {
        id: filterField
        fieldName: "profile-move-filter"
        label: I18n.t("moveProfile.field")
        placeholderText: I18n.t("moveProfile.ph")
        onTextChanged: { root.query = filterField.text; root.current = 0; }
        onAccepted: root.submit()
        // The field keeps the keyboard; the arrows walk the list from it.
        Keys.onDownPressed: root.move(1)
        Keys.onUpPressed: root.move(-1)
    }

    Text {
        objectName: "profile-move-none"
        visible: root.choices.length === 0
        Layout.fillWidth: true
        text: I18n.t("moveProfile.none")
        color: Theme.textDim
        font.family: Theme.fontUi
        font.pixelSize: Theme.fsMd
        wrapMode: Text.Wrap
    }

    ColumnLayout {
        objectName: "profile-move-list"
        visible: root.choices.length > 0
        Layout.fillWidth: true
        spacing: 0
        Repeater {
            model: root.choices
            delegate: Item {
                id: row
                required property var modelData
                required property int index
                readonly property bool on: row.index === root.current
                objectName: "profile-move-row-" + row.modelData.id
                Layout.fillWidth: true
                implicitHeight: Theme.px(32)
                Rectangle {
                    anchors.fill: parent
                    radius: Theme.radiusSm
                    color: row.on ? Theme.panel3 : "transparent"
                }
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.spMd
                    anchors.rightMargin: Theme.spMd
                    spacing: Theme.spMd
                    Rectangle {
                        implicitWidth: Theme.spSm
                        implicitHeight: Theme.spSm
                        radius: width / 2
                        color: Theme.profileInk(row.modelData.color)
                    }
                    Text {
                        Layout.fillWidth: true
                        text: row.modelData.name || ""
                        elide: Text.ElideRight
                        color: Theme.text
                        font.family: Theme.fontUi
                        font.pixelSize: Theme.fsMd
                        font.weight: row.on ? Theme.fwTitle : Theme.fwBody
                    }
                    Text {
                        text: I18n.count(row.modelData.tasks || 0, "moveProfile.count")
                        color: Theme.textDim
                        font.family: Theme.fontUi
                        font.features: Theme.tabularNums
                        font.pixelSize: Theme.fsXs
                    }
                }
                ClickArea {
                    label: row.modelData.name || ""
                    showTip: false
                    onActivated: { root.current = row.index; root.submit(); }
                }
            }
        }
    }

    Text {
        objectName: "profile-move-tracker"
        visible: root._trackerKeys.length > 0
        Layout.fillWidth: true
        text: I18n.t("moveProfile.trackerNote").arg(root._trackerKeys.slice(0, 3).join(", "))
        color: Theme.textDim
        font.family: Theme.fontUi
        font.pixelSize: Theme.fsSm
        wrapMode: Text.Wrap
    }

    buttons: [
        PillButton {
            objectName: "profile-move-cancel"
            text: I18n.t("common.cancel")
            onClicked: root.close()
        },
        PillButton {
            objectName: "profile-move-ok"
            enabled: root.choices.length > 0
            text: I18n.t("moveProfile.move")
            onClicked: root.submit()
        }
    ]
}
