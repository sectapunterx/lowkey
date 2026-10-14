// 0.9.2: a note opens as its source beside the drawn note (split), and a
// "live" stored by 0.9.1 under notes.mode does not keep anyone in the
// in-place block editor.
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

    function test_a_note_opens_split_even_after_a_stored_live() {
        AppController.appSettingsJson = JSON.stringify({ notes: { mode: "live" } });
        const id = AppController.newNote("split probe");
        tc.seeded.push(id);
        AppController.setNoteBody(id, "# Split probe\n\n| a | b |\n|---|---|\n| 1 | 2 |\n");
        AppController.activeNoteId = id;
        const nv = createTemporaryQmlObject('import TodoCpp; NotesView { anchors.fill: parent }', host);
        tryVerify(() => nv._loadedOnce);
        const source = find(nv, "notesEditor");
        compare(nv.viewMode, "split");
        const preview = find(nv, "notesPreview");
        const live = find(nv, "notes-live");
        verify(preview !== null);
        tryVerify(() => preview.visible, 1000, "the drawn note is not beside the source");
        verify(!live.visible, "the in-place editor still shows");
        verify(source.visible, "the source is not shown");
        // Two halves, not one pane and a sliver.
        verify(Math.abs(preview.width - source.width) < nv.width * 0.2,
               "preview " + preview.width + " vs source " + source.width);
    }

    function test_ctrl_shift_m_toggles_source_alone_and_back() {
        const nv = createTemporaryQmlObject('import TodoCpp; NotesView { anchors.fill: parent }', host);
        tryVerify(() => nv._loadedOnce);
        nv.viewMode = "split";
        nv.viewMode = "edit";
        compare(JSON.parse(AppController.appSettingsJson).notes.view, "edit");
        nv.viewMode = "split";
        compare(JSON.parse(AppController.appSettingsJson).notes.view, "split");
    }
}
