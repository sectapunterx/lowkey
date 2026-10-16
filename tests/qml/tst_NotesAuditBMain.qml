// SHELL-1 through Main: the palette lists the notes.* actions and hands them
// to Main.runCommand, which used to log "palette: no command" and do nothing.
import QtQuick
import QtTest
import TodoCpp

TestCase {
    id: tc
    name: "NotesAuditBMain"
    when: windowShown

    property var win: null

    function initTestCase() {
        const comp = Qt.createComponent("qrc:/qt/qml/TodoCpp/qml/Main.qml");
        tryCompare(comp, "status", Component.Ready, 5000);
        tc.win = comp.createObject(null);
        verify(tc.win !== null);
        tc.win.width = 1456;
        tc.win.height = 939;
        wait(1200);
    }
    function cleanupTestCase() {
        AppController.currentView = "board";
        if (tc.win) tc.win.destroy();
    }

    function test_palette_notes_new_makes_a_note() {
        AppController.currentView = "board";
        const count = AppController.notes.rowCount();

        tc.win.runCommand("notes.new");

        tryCompare(AppController, "currentView", "notes");
        tryVerify(function () { return AppController.notes.rowCount() === count + 1; }, 3000,
                  "notes.new from the palette did nothing");
        AppController.deleteNote(AppController.activeNoteId);
        AppController.clearPendingUndo();
    }

    // Dogfood 2026-10-16: Ctrl+Alt+N did nothing outside Notes. From the
    // board it goes to Notes and starts a note.
    function test_ctrl_alt_n_from_the_board() {
        AppController.currentView = "board";
        wait(200);
        tc.win.requestActivate();
        tc.win.focusActiveView();
        const count = AppController.notes.rowCount();
        keyClick(Qt.Key_N, Qt.ControlModifier | Qt.AltModifier);
        tryCompare(AppController, "currentView", "notes");
        tryVerify(function () { return AppController.notes.rowCount() === count + 1; }, 3000,
                  "Ctrl+Alt+N from the board made no note");
        AppController.deleteNote(AppController.activeNoteId);
        AppController.clearPendingUndo();
    }

    // APP-320: the label chips over the notes list filter by the tag.
    function test_label_chips_filter() {
        AppController.currentView = "notes";
        const id = AppController.appendNoteEntry("labelchip probe", "chipprobe");
        const pane = findChild(tc.win.contentItem, "notes-tag-chips");
        verify(pane !== null);
        tryVerify(function () { return findChild(pane, "notes-tag-chipprobe") !== null; }, 3000, "no chip for the label");
        const chip = findChild(pane, "notes-tag-chipprobe");
        const ca = chip.children[chip.children.length - 1];
        ca.activated();
        const field = findChild(tc.win.contentItem, "note-filter");
        compare(field.text, "#chipprobe");
        ca.activated();
        compare(field.text, "");
        AppController.deleteNote(id);
        AppController.clearPendingUndo();
    }
}
