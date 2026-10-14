// How a note opens. Stable 0.9.2 opens it split (the source beside the drawn
// note); feature opens it rich: one editor, the markup hidden away from the
// caret (Obsidian's live preview). A "live" stored by 0.9.1 under notes.mode
// keeps nobody in the in-place block editor.
import QtQuick
import QtTest
import TodoCpp

TestCase {
    id: tc
    name: "NotesSplit"
    when: windowShown
    visible: true
    width: 1100
    height: 700

    Item { id: host; anchors.fill: parent }
    property var seeded: []

    function cleanup() {
        for (let i = 0; i < tc.seeded.length; i++) AppController.deleteNote(tc.seeded[i]);
        tc.seeded = [];
    }
    function find(root, name) {
        if (!root) return null;
        if (root.objectName === name) return root;
        const kids = root.children || [];
        for (let i = 0; i < kids.length; i++) {
            const f = find(kids[i], name);
            if (f) return f;
        }
        return null;
    }
    function open(body) {
        const id = AppController.newNote("view probe");
        tc.seeded.push(id);
        AppController.setNoteBody(id, body);
        AppController.activeNoteId = id;
        const nv = createTemporaryQmlObject('import TodoCpp; NotesView { anchors.fill: parent }', host);
        tryVerify(() => nv._loadedOnce);
        return nv;
    }

    function test_a_note_opens_rich_even_after_a_stored_live() {
        AppController.appSettingsJson = JSON.stringify({ notes: { mode: "live" } });
        const nv = open("# Probe\n\nSome **bold** text.\n");
        compare(nv.viewMode, "rich");
        const source = find(nv, "notesEditor");
        verify(source.visible, "the note's text is not shown");
        verify(!find(nv, "notes-live").visible, "the in-place block editor still shows");
        verify(!find(nv, "notesPreview").visible, "rich is one editor, not two panes");
        // Ctrl+A takes the note's text but not its title; again, everything.
        source.forceActiveFocus();
        keySequence(StandardKey.SelectAll);
        compare(source.selectedText, "Some **bold** text.\n");
        keySequence(StandardKey.SelectAll);
        compare(source.selectedText.indexOf("# Probe"), 0);
        // Shift+Down runs on from line to line: the note is one text.
        source.cursorPosition = 0;
        keyClick(Qt.Key_Down, Qt.ShiftModifier);
        keyClick(Qt.Key_Down, Qt.ShiftModifier);
        verify(source.selectedText.indexOf("# Probe") === 0 && source.selectedText.length > "# Probe".length,
               JSON.stringify(source.selectedText));
    }

    function test_split_shows_two_halves() {
        AppController.appSettingsJson = JSON.stringify({ notes: { view: "split" } });
        const nv = open("# Split probe\n\n| a | b |\n|---|---|\n| 1 | 2 |\n");
        compare(nv.viewMode, "split");
        const preview = find(nv, "notesPreview");
        const source = find(nv, "notesEditor");
        tryVerify(() => preview.visible, 1000, "the drawn note is not beside the source");
        verify(source.visible);
        tryVerify(() => preview.width > 0 && Math.abs(preview.width - source.width) < nv.width * 0.2, 1000,
                  "preview " + preview.width + " vs source " + source.width);
    }

    function test_ctrl_shift_m_steps_rich_split_edit() {
        AppController.appSettingsJson = JSON.stringify({ notes: { view: "rich" } });
        const nv = open("text");
        compare(nv.viewMode, "rich");
        nv.forceActiveFocus();
        keyClick(Qt.Key_M, Qt.ControlModifier | Qt.ShiftModifier);
        compare(nv.viewMode, "split");
        keyClick(Qt.Key_M, Qt.ControlModifier | Qt.ShiftModifier);
        compare(nv.viewMode, "edit");
        keyClick(Qt.Key_M, Qt.ControlModifier | Qt.ShiftModifier);
        compare(nv.viewMode, "rich");
        compare(JSON.parse(AppController.appSettingsJson).notes.view, "rich");
    }
}
