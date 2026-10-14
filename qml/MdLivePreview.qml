pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic as QQC
import TodoCpp

// Rich mode for one markdown text area (APP-313): the highlighter over its
// source and the two layers drawn around it, so the note, the task
// document, the task dialog and the tracker's text all draw the same way.
// Off `rich`, the highlighter only colours the source (Notes' split and edit).
//
// Put it beside the editor: the layers go into the editor's parent, the code
// box under the text and the rest over it.
Item {
    id: root

    required property QQC.TextArea editor
    property bool rich: true
    // Read-only text (the tracker's): all of it drawn, boxes not clickable.
    property bool readOnly: false
    property real headingScale: 1.45

    readonly property alias highlighter: hl
    readonly property var decorations: root.rich ? hl.decorations(root.editor.text) : []
    // The caret / selection range the source shows at; -1 when nothing is.
    readonly property int revealStart: hl.revealStart
    readonly property int revealEnd: hl.revealEnd

    width: 0
    height: 0

    MdLiveDecorations {
        parent: root.editor.parent
        editor: root.editor
        under: true
        z: root.editor.z - 1
        visible: root.rich && root.editor.visible
        decorations: root.decorations
        revealStart: hl.revealStart
        revealEnd: hl.revealEnd
    }
    MdLiveDecorations {
        objectName: "md-live-decorations"
        parent: root.editor.parent
        editor: root.editor
        z: root.editor.z + 1
        visible: root.rich && root.editor.visible
        tasksClickable: !root.readOnly
        decorations: root.decorations
        revealStart: hl.revealStart
        revealEnd: hl.revealEnd
    }

    // Markup hidden away from the caret in rich; the line (or the lines of
    // the selection) being edited shows its source. Nothing is revealed while
    // the editor does not hold the keyboard, nor ever in read-only text.
    MdHighlighter {
        id: hl
        headingScale: root.headingScale
        livePreview: root.rich
        revealStart: root.editor.activeFocus && !root.readOnly ? root.editor.selectionStart : -1
        revealEnd: root.editor.activeFocus && !root.readOnly ? root.editor.selectionEnd : -1
        palette: ({
            text:          Theme.text,
            dim:           Theme.textMuted,
            accent:        Theme.accent,
            code:          Theme.code,
            codeBg:        Theme.codeBg,
            mention:       Theme.mention,
            ticket:        Theme.ticket,
            tag:           Theme.tag,
            math:          Theme.math,
            highlightBg:   Theme.highlightBg,
            keyword:       Theme.synKeyword,
            string:        Theme.synString,
            number:        Theme.synNumber
        })
    }
    // Wired here, not bound: a binding can fire before the text area's
    // document is ready.
    Component.onCompleted: hl.target = root.editor.textDocument
}
