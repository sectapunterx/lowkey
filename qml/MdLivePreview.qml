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
//
// Ctrl+click opens what is under the pointer, as a click does in a drawn
// note: an address through LinkConfirmDialog, a file attached here, a
// footnote's text below; a note, a task, a tag or a person is handed to the
// host as internalLinkActivated.
Item {
    id: root

    required property QQC.TextArea editor
    property bool rich: true
    // Read-only text (the tracker's): all of it drawn, boxes not clickable.
    property bool readOnly: false
    property real headingScale: 1.45

    signal internalLinkActivated(string kind, string target)

    readonly property alias highlighter: hl
    readonly property var decorations: root.rich ? hl.decorations(root.editor.text, AppController.taskTitles) : []
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
    // Ctrl+click on a link, between the text and what is drawn over it. Any
    // other press goes on to the text.
    MouseArea {
        objectName: "md-live-link-area"
        parent: root.editor.parent
        x: root.editor.x
        y: root.editor.y
        z: root.editor.z + 0.5
        width: root.editor.width
        height: root.editor.height
        visible: root.editor.visible
        cursorShape: Qt.IBeamCursor
        onPressed: (mouse) => {
            if (!(mouse.modifiers & Qt.ControlModifier)) {
                mouse.accepted = false;
                return;
            }
            const link = root.linkAt(mouse.x, mouse.y);
            if (link.length === 0) {
                mouse.accepted = false;
                return;
            }
            root.open(link);
        }
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

    // The link under a point of the editor, "" for none.
    function linkAt(x: real, y: real): string {
        return hl.linkAt(root.editor.text, root.editor.positionAt(x, y), AppController.taskTitles);
    }

    // As MdView opens a link it draws.
    function open(link: string) {
        if (link.startsWith("heap://")) {
            const rest = link.substring(7);
            const slash = rest.indexOf("/");
            if (slash <= 0) return;
            const kind = rest.substring(0, slash);
            let target = rest.substring(slash + 1);
            try { target = decodeURIComponent(target); } catch (e) { /* keep it as written */ }
            if (kind === "fn") {
                root._toFootnote(target);
                return;
            }
            root.internalLinkActivated(kind, target);
            return;
        }
        const att = /^attachments\/([0-9a-f]{32}(?:\.[a-z0-9]{1,12})?)$/.exec(link);
        if (att) {
            attOpener.open(att[1], att[1]);
            return;
        }
        linkConfirm.openLink(link);
    }

    // The caret to the footnote's own line ("[^id]: …").
    function _toFootnote(id: string) {
        const t = root.editor.text;
        const mark = "[^" + id + "]:";
        const at = t.startsWith(mark) ? 0 : t.indexOf("\n" + mark);
        if (at < 0) return;
        root.editor.forceActiveFocus();
        root.editor.cursorPosition = at === 0 ? 0 : at + 1;
    }

    LinkConfirmDialog { id: linkConfirm }
    // Holds the "open this file?" dialog for attachment links.
    AttachmentChips { id: attOpener; visible: false; model: []; removable: false }

    // Markup hidden away from the caret in rich; the line (or the lines of
    // the selection) being edited shows its source. Nothing is revealed while
    // the editor does not hold the keyboard, nor ever in read-only text.
    MdHighlighter {
        id: hl
        headingScale: root.headingScale
        livePreview: root.rich
        monoFamily: Theme.fontMono
        // Headings in the headings' face, as the drawn note has them.
        displayFamily: root.rich ? Theme.fontDisplay : ""
        ticketTitles: root.rich ? AppController.taskTitles : ({})
        revealStart: root.editor.activeFocus && !root.readOnly ? root.editor.selectionStart : -1
        revealEnd: root.editor.activeFocus && !root.readOnly ? root.editor.selectionEnd : -1
        palette: ({
            text:          Theme.text,
            dim:           Theme.textMuted,
            accent:        Theme.accent,
            code:          Theme.code,
            // code in a line and <kbd>: the theme's own colour.
            inlineCode:    Theme.accent,
            codeBg:        Theme.codeBg,
            mention:       Theme.mention,
            ticket:        Theme.ticket,
            tag:           Theme.tag,
            // Formulas in the theme's own colour, as code is.
            math:          Theme.accent,
            highlightBg:   Theme.highlightBg,
            keyword:       Theme.synKeyword,
            string:        Theme.synString,
            number:        Theme.synNumber,
            // A log's levels (APP-315).
            error:         Theme.danger,
            warn:          Theme.warning
        })
    }
    // Wired here, not bound: a binding can fire before the text area's
    // document is ready.
    Component.onCompleted: hl.target = root.editor.textDocument
}
