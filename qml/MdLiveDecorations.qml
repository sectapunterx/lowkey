pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic as QQC
import TodoCpp

// What live preview draws over the note's hidden source (Obsidian-style): a
// bullet on a "- ", a box on a "- [ ]", a bar beside a quote, a line over a
// "---", a grid over a table, a box behind fenced code. The source stays in
// the editor as the one text to select, copy and edit; an element the caret
// (or the selection) touches is not drawn and its source shows.
//
// Two layers so the code box sits under the text and the rest over it: set
// `under` on the one placed before the editor.
Item {
    id: root

    required property QQC.TextArea editor
    // MdHighlighter.decorations(editor.text).
    property var decorations: []
    // The caret / selection range; -1 when the editor does not hold the
    // keyboard (then everything is drawn).
    property int revealStart: -1
    property int revealEnd: -1
    property bool under: false
    // Off for read-only text: a box is drawn, a click does not tick it.
    property bool tasksClickable: true

    x: editor.x
    y: editor.y
    width: editor.width
    height: editor.height

    // Positions move with the layout, which says nothing when a line's
    // revealed markup makes it wrap: a tick after every change re-reads them.
    property int _rev: 0
    function _bump() { root._rev++; }
    Connections {
        target: root.editor
        function onContentHeightChanged() { Qt.callLater(root._bump); }
        function onWidthChanged() { Qt.callLater(root._bump); }
        function onTextChanged() { Qt.callLater(root._bump); }
        function onCursorPositionChanged() { Qt.callLater(root._bump); }
    }
    function _rect(pos: int): rect {
        root._rev;
        return root.editor.positionToRectangle(pos);
    }
    function _shown(d: var): bool {
        if (root.revealStart < 0) return true;
        const a = Math.min(root.revealStart, root.revealEnd), b = Math.max(root.revealStart, root.revealEnd);
        return !(a <= d.end && b >= d.start);
    }

    FontMetrics { id: fm; font: root.editor.font }
    // A header cell is drawn bold, so it is measured bold.
    FontMetrics {
        id: fmHead
        font.family: root.editor.font.family
        font.pixelSize: root.editor.font.pixelSize
        font.weight: Theme.fwTitle
    }
    // Off-screen, only to reach the clipboard (as MdView's code blocks do).
    TextEdit { id: clipboardHelper; visible: false }

    Repeater {
        model: root.decorations
        delegate: Loader {
            id: deco
            required property var modelData
            // Code: the box under the text, its language and Copy over it.
            active: root._shown(modelData) && (!root.under || modelData.kind === "code"
                                               || (modelData.kind === "quote" && String(modelData.callout || "").length > 0))
            sourceComponent: {
                if (deco.modelData.kind === "code" && !root.under) return codeHeader;
                if (deco.modelData.kind === "quote" && root.under) return calloutBg;
                switch (deco.modelData.kind) {
                case "bullet": return bullet;
                case "task": return task;
                case "quote": return quote;
                case "rule": return rule;
                case "code": return code;
                case "table": return table;
                case "math": return deco.modelData.display ? mathBlock : mathInline;
                case "ticket": return ticketRef;
                case "footnote": return footnoteRef;
                }
                return null;
            }

            Component {
                id: bullet
                Text {
                    readonly property rect r: root._rect(deco.modelData.at)
                    x: r.x
                    y: r.y
                    height: r.height
                    verticalAlignment: Text.AlignVCenter
                    text: deco.modelData.depth % 2 === 0 ? "•" : "◦"
                    color: Theme.textMuted
                    font: root.editor.font
                }
            }
            Component {
                id: task
                Rectangle {
                    objectName: "md-live-task"
                    readonly property rect r: root._rect(deco.modelData.at)
                    readonly property bool checked: deco.modelData.checked
                    x: r.x
                    y: r.y + (r.height - height) / 2
                    width: Theme.px(14)
                    height: Theme.px(14)
                    radius: Theme.radiusSm
                    color: checked ? Theme.accent : "transparent"
                    border.width: 1
                    border.color: checked ? Theme.accent : Theme.textMuted
                    Text {
                        anchors.centerIn: parent
                        visible: parent.checked
                        text: "✓"
                        color: Theme.textOnAccent
                        font.pixelSize: Theme.fsSm
                    }
                    // A click ticks it in the source: one edit, one undo step.
                    // After the click is over: the edit rebuilds these items,
                    // and a handler destroyed inside its own tap kept the
                    // pointer, so later clicks never reached the editor.
                    MouseArea {
                        enabled: root.tasksClickable
                        anchors.fill: parent
                        anchors.margins: -Theme.spXs
                        cursorShape: Qt.PointingHandCursor
                        onClicked: Qt.callLater(root.toggleTask, deco.modelData.box, parent.checked)
                    }
                }
            }
            Component {
                id: quote
                Rectangle {
                    readonly property rect r: root._rect(deco.modelData.start)
                    x: r.x
                    y: r.y
                    width: Theme.px(3)
                    height: root._rect(deco.modelData.end).y + r.height - r.y
                    radius: width / 2
                    color: String(deco.modelData.callout || "").length > 0 ? root.calloutColor(deco.modelData.callout) : Theme.border
                }
            }
            // A callout's tint, under its lines ("> [!note]" and the quote under it).
            Component {
                id: calloutBg
                Rectangle {
                    objectName: "md-live-callout"
                    readonly property rect r: root._rect(deco.modelData.start)
                    x: -Theme.spSm
                    y: r.y
                    width: root.width + 2 * Theme.spSm
                    height: root._rect(deco.modelData.end).y + r.height - r.y
                    color: Theme.withAlpha(root.calloutColor(deco.modelData.callout), 0.08)
                }
            }
            // A formula (MdMath, APP-312) over its hidden source, which the
            // highlighter squeezed to the drawn width so the line stays put.
            Component {
                id: mathInline
                Text {
                    objectName: "md-live-math"
                    readonly property rect r: root._rect(deco.modelData.at)
                    x: r.x
                    y: r.y
                    height: r.height
                    verticalAlignment: Text.AlignVCenter
                    textFormat: Text.RichText
                    text: deco.modelData.html
                    color: Theme.accent
                    font: root.editor.font
                }
            }
            // A footnote's number, small and raised, over its hidden "[^1]".
            Component {
                id: footnoteRef
                Text {
                    objectName: "md-live-footnote"
                    readonly property rect r: root._rect(deco.modelData.at)
                    x: r.x + 1
                    y: r.y
                    text: deco.modelData.label
                    textFormat: Text.PlainText
                    color: Theme.accent
                    font.family: root.editor.font.family
                    font.pixelSize: Math.max(1, Math.round(root.editor.font.pixelSize * 0.7))
                }
            }
            // A known task's title over its "#jira-APP-1" (hidden, as wide as
            // the title), as the drawn note reads it. Ctrl+click opens it.
            Component {
                id: ticketRef
                Text {
                    objectName: "md-live-ticket"
                    readonly property rect r: root._rect(deco.modelData.at)
                    x: r.x
                    y: r.y
                    height: r.height
                    verticalAlignment: Text.AlignVCenter
                    text: deco.modelData.label
                    textFormat: Text.PlainText
                    color: Theme.ticket
                    font: root.editor.font
                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: Theme.px(3)
                        height: 1
                        color: Theme.ticket
                    }
                }
            }
            // $$ … $$: centred over its lines.
            Component {
                id: mathBlock
                Item {
                    readonly property rect a: root._rect(deco.modelData.start)
                    readonly property rect b: root._rect(deco.modelData.end)
                    x: 0
                    y: a.y
                    width: root.width
                    height: b.y + b.height - a.y
                    Text {
                        objectName: "md-live-math-block"
                        anchors.centerIn: parent
                        width: Math.min(implicitWidth, parent.width)
                        wrapMode: Text.Wrap
                        horizontalAlignment: Text.AlignHCenter
                        textFormat: Text.RichText
                        text: deco.modelData.html
                        color: Theme.accent
                        font: root.editor.font
                    }
                }
            }
            Component {
                id: rule
                Rectangle {
                    readonly property rect r: root._rect(deco.modelData.start)
                    x: 0
                    y: r.y + r.height / 2
                    width: root.width
                    height: 1
                    color: Theme.border
                }
            }
            Component {
                id: code
                Rectangle {
                    readonly property rect a: root._rect(deco.modelData.start)
                    readonly property rect b: root._rect(deco.modelData.end)
                    x: -Theme.spSm
                    y: a.y - Theme.spXs
                    width: root.width + 2 * Theme.spSm
                    height: b.y + b.height - a.y + 2 * Theme.spXs
                    radius: Theme.radiusMd
                    color: Theme.codeBg
                    border.width: 1
                    border.color: Theme.border
                }
            }
            // On the hidden ``` line (or the box's first line, for indented
            // code): the language, "not closed" for a fence that runs to the
            // end of the note, and Copy, which takes the code without fences.
            Component {
                id: codeHeader
                Item {
                    readonly property rect a: root._rect(deco.modelData.start)
                    x: 0
                    y: a.y
                    width: root.width
                    height: a.height
                    Text {
                        objectName: "md-live-code-language"
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        visible: deco.modelData.fenced
                        text: !deco.modelData.closed ? I18n.t("notes.code.unclosed")
                            : String(deco.modelData.language || "")
                        color: deco.modelData.closed ? Theme.textDim : Theme.warning
                        font.family: Theme.fontMono
                        font.pixelSize: Theme.fsXs
                    }
                    Text {
                        id: copyText
                        objectName: "md-live-code-copy"
                        property bool copied: false
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        text: copied ? I18n.t("notes.code.copied") : I18n.t("notes.code.copy")
                        color: copyArea.hovered ? Theme.text : Theme.textDim
                        font.family: Theme.fontUi
                        font.pixelSize: Theme.fsXs
                        ClickArea {
                            id: copyArea
                            label: I18n.t("notes.code.copy")
                            onActivated: {
                                clipboardHelper.text = deco.modelData.code;
                                clipboardHelper.selectAll();
                                clipboardHelper.copy();
                                copyText.copied = true;
                                copiedReset.restart();
                            }
                        }
                        Timer { id: copiedReset; interval: 1200; onTriggered: copyText.copied = false }
                    }
                }
            }
            Component {
                id: table
                Item {
                    id: grid
                    readonly property var rows: deco.modelData.rows
                    readonly property var align: deco.modelData.align
                    // Column widths from the widest cell, as a drawn table has them.
                    readonly property var widths: {
                        const w = [];
                        for (const row of grid.rows) {
                            if (row.separator) continue;
                            for (let c = 0; c < row.cells.length; c++)
                                w[c] = Math.max(w[c] || 0, Math.ceil((row.header ? fmHead : fm).advanceWidth(root.plain(row.cells[c]))) + 2 * Theme.spLg + Theme.spSm);
                        }
                        return w;
                    }
                    Repeater {
                        model: grid.rows
                        delegate: Rectangle {
                            id: rowBox
                            required property var modelData
                            required property int index
                            readonly property rect r: root._rect(modelData.start)
                            // The |---| line under the header is part of the
                            // header's box, not an empty row.
                            readonly property rect next: index + 1 < grid.rows.length && grid.rows[index + 1].separator
                                                         ? root._rect(grid.rows[index + 1].start) : r
                            visible: !modelData.separator
                            x: 0
                            y: r.y
                            height: next.y + next.height - r.y
                            width: Math.max(1, grid.widths.reduce((s, v) => s + v, 0))
                            color: modelData.header ? Theme.panel : Theme.bg
                            border.width: 1
                            border.color: Theme.border
                            Row {
                                anchors.fill: parent
                                Repeater {
                                    model: rowBox.modelData.cells
                                    delegate: Rectangle {
                                        required property string modelData
                                        required property int index
                                        width: grid.widths[index] || 0
                                        height: rowBox.height
                                        color: "transparent"
                                        border.width: 1
                                        border.color: Theme.border
                                        Text {
                                            anchors.fill: parent
                                            anchors.leftMargin: Theme.spLg
                                            anchors.rightMargin: Theme.spLg
                                            verticalAlignment: Text.AlignVCenter
                                            horizontalAlignment: grid.align[parent.index] === "r" ? Text.AlignRight
                                                               : grid.align[parent.index] === "c" ? Text.AlignHCenter : Text.AlignLeft
                                            text: root.plain(parent.modelData)
                                            elide: Text.ElideRight
                                            color: Theme.text
                                            font.family: root.editor.font.family
                                            font.pixelSize: root.editor.font.pixelSize
                                            font.weight: rowBox.modelData.header ? Theme.fwTitle : Theme.fwBody
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // "[ ]" <-> "[x]" at the box, the keyboard back in the note.
    function toggleTask(box: int, checked: bool) {
        const p = box + 1;
        const c = root.editor.cursorPosition;
        root.editor.remove(p, p + 1);
        root.editor.insert(p, checked ? " " : "x");
        root.editor.cursorPosition = c;
        root.editor.forceActiveFocus();
    }

    // A callout's colour by its kind, as MdView draws it: the priority and
    // status colours the app already speaks; anything else the accent.
    function calloutColor(kind: string): color {
        switch (String(kind).toLowerCase()) {
        case "warning": case "caution": case "attention": return Theme.warning;
        case "danger": case "error": case "bug": return Theme.danger;
        case "tip": case "success": case "done": case "check": return Theme.success;
        default: return Theme.accent;
        }
    }

    // A cell's text without its inline markup.
    function plain(s: string): string {
        return String(s).replace(/\*\*|__|~~|==|`/g, "").replace(/\[([^\]]*)\]\([^)]*\)/g, "$1").replace(/\[\[([^\]|]*\|)?([^\]]*)\]\]/g, "$2");
    }
}
