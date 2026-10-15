pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import TodoCpp

// The git "working on" line (a-11, N/X-Oth-Knowledge): a plain 32px row over
// the view with a hairline under it — "работаю над", the task's ID, the
// branch in mono, "PR #482 · CI прошёл", ×. Only the fact and a link to the
// task: no pill, no framed badges, no "Открыть" button (the ID opens the
// task, the PR opens the pull request).
Item {
    id: root
    objectName: "git-working-line"

    property string taskId: ""
    property string branch: ""
    // The focused repo's PR ({ number, state, draft, url, checks }) or null.
    property var pr: null
    signal openTask()
    signal dismiss()

    readonly property bool _hasPr: !!(root.pr && String(root.pr.state || "").length > 0
                                       && Number(root.pr.number || 0) > 0)
    readonly property string _ci: root.pr ? String(root.pr.checks || "") : ""
    // "PR #482 · CI прошёл": an open PR is just its number; merged, closed
    // and draft say so. Words, not glyphs (DG-003).
    readonly property string prText: {
        const parts = [];
        if (root._hasPr) {
            const s = String(root.pr.state || "");
            let t = "PR #" + root.pr.number;
            if (s === "merged" || s === "closed") t += " " + I18n.t("topbar.pr." + s);
            if (root.pr.draft === true) t += " " + I18n.t("topbar.pr.draft");
            parts.push(t);
        }
        if (root._ci.length > 0)
            parts.push("CI " + I18n.t(root._ci === "passing" ? "topbar.ci.passing"
                                      : root._ci === "failing" ? "topbar.ci.failing" : "topbar.ci.running"));
        return parts.join(" · ");
    }

    implicitHeight: Theme.px(32)
    implicitWidth: row.implicitWidth + row.anchors.leftMargin + row.anchors.rightMargin

    Rectangle {
        objectName: "git-working-line-rule"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        color: Theme.border
    }

    RowLayout {
        id: row
        anchors.fill: parent
        anchors.leftMargin: Theme.spXs
        anchors.rightMargin: Theme.spXs
        spacing: Theme.spXl

        Text {
            text: I18n.t("topbar.git.workingOn")
            color: Theme.textMuted
            font.family: Theme.fontUi
            font.pixelSize: Theme.fsXs
        }
        Text {
            objectName: "git-working-line-task"
            text: root.taskId
            color: taskCA.hovered ? Theme.accentStrong : Theme.text
            Behavior on color { ColorAnimation { duration: Theme.durTap; easing.type: Theme.easeEnter } }
            font.family: Theme.fontUi
            font.pixelSize: Theme.fsXs
            ClickArea {
                id: taskCA
                objectName: "topbar-git-open"
                label: I18n.t("topbar.git.open") + " " + root.taskId
                tip: I18n.t("topbar.git.open")
                onActivated: root.openTask()
            }
        }
        Text {
            objectName: "git-working-line-branch"
            visible: root.branch.length > 0 && root.branch !== "(detached HEAD)"
            Layout.fillWidth: true
            Layout.maximumWidth: implicitWidth
            text: root.branch
            elide: Text.ElideMiddle
            color: Theme.textMuted
            font.family: Theme.fontMono
            font.pixelSize: Theme.px(11)
        }
        Text {
            objectName: "git-working-line-pr"
            visible: root.prText.length > 0
            text: root.prText
            color: prCA.hovered ? Theme.text : Theme.textMuted
            Behavior on color { ColorAnimation { duration: Theme.durTap; easing.type: Theme.easeEnter } }
            font.family: Theme.fontUi
            font.features: Theme.tabularNums
            font.pixelSize: Theme.fsXs
            ClickArea {
                id: prCA
                objectName: "topbar-pr-badge"
                enabled: !!(root._hasPr && root.pr.url)
                label: parent.text
                tip: I18n.t("topbar.pr.openTip")
                onActivated: Qt.openUrlExternally(root.pr.url)
            }
        }
        Item { Layout.fillWidth: true }
        // Dismiss: the × has a 20px hit area, not the glyph's own bounds.
        Item {
            Layout.preferredWidth: Theme.px(20)
            Layout.preferredHeight: Theme.px(20)
            Icon {
                anchors.centerIn: parent
                name: "close"
                size: Theme.px(12)
                color: dismissMA.hovered ? Theme.text : Theme.textDim
                Behavior on color { ColorAnimation { duration: Theme.durTap; easing.type: Theme.easeEnter } }
            }
            ClickArea {
                id: dismissMA
                objectName: "topbar-git-dismiss"
                label: I18n.t("topbar.git.dismiss")
                onActivated: root.dismiss()
            }
        }
    }
}
