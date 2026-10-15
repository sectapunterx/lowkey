// The rich editor against the Knowledge and Task sheets (design gaps 0.9.4):
// the note's leading (a-02), "/" on an empty line (a-03), the list's hanging
// indent (a-04), a task as a pill (a-05), `code` in a pill (a-06, b-02), the
// picture with its caption (a-09), the code block's header (a-10) and a
// link's dim underline (b-25).
import QtQuick
import QtTest
import TodoCpp

TestCase {
    id: tc
    name: "RichEditorLook"
    when: windowShown
    visible: true
    width: 1000
    height: 760

    Item { id: host; anchors.fill: parent }

    property var notes: []
    property var made: []
    function init() { AppController.clearPendingUndo(); }
    function cleanup() {
        for (const id of tc.notes) AppController.deleteNote(id);
        tc.notes = [];
        for (const id of tc.made) AppController.deleteTask(id);
        tc.made = [];
        AppController.clearPendingUndo();
    }
    function note(title, body) {
        const id = AppController.newNote(title);
        AppController.setNoteBody(id, body);
        tc.notes.push(id);
        return id;
    }
    function mkTask(title, desc) {
        const d = AppController.newTaskDraft("todo");
        d._isNew = true;
        d.title = title;
        d.desc = desc || "";
        AppController.saveTask(d);
        tc.made.push(d.id);
        return d.id;
    }
    function richNotes(id) {
        AppController.activeNoteId = id;
        const nv = createTemporaryQmlObject('import TodoCpp; NotesView { width: 1000; height: 740 }', host);
        tryVerify(() => nv._loadedOnce);
        nv.viewMode = "rich";
        const ed = findChild(nv, "notesEditor");
        tryVerify(() => ed.visible);
        return nv;
    }
    function decorated(nv, name) {
        const all = [];
        const walk = (item) => {
            if (!item) return;
            if (item.objectName === name && item.visible) all.push(item);
            const kids = item.children || [];
            for (let i = 0; i < kids.length; i++) walk(kids[i]);
        };
        walk(nv);
        return all;
    }

    // a-02: a note's lines are 1.7 times its text apart.
    function test_note_text_reads_at_one_point_seven() {
        const nv = richNotes(note("Leading probe", "first line\nsecond line\nthird line"));
        const ed = findChild(nv, "notesEditor");
        const want = Math.round(ed.font.pixelSize * 1.7);
        tryVerify(() => Math.abs(ed.positionToRectangle(11).y - ed.positionToRectangle(0).y - want) <= 1, 2000,
                  "lines " + (ed.positionToRectangle(11).y - ed.positionToRectangle(0).y) + " apart, want " + want);
    }

    // a-03: "/" on an empty line is the insert menu; on a line of text, a "/".
    function test_slash_on_an_empty_line_opens_the_insert_menu() {
        const nv = richNotes(note("Slash probe", "words\n"));
        const ed = findChild(nv, "notesEditor");
        const menu = findChild(nv, "notes-slash-menu");
        verify(menu !== null, "no insert menu in the rich editor");
        ed.forceActiveFocus();
        ed.cursorPosition = 3;
        keyClick(Qt.Key_Slash);
        compare(ed.text, "wor/ds\n", "inside a line it is a character");
        verify(!menu.opened);
        ed.cursorPosition = ed.length;
        keyClick(Qt.Key_Slash);
        tryVerify(() => menu.opened, 1000, "the menu did not open on the empty line: " + JSON.stringify(ed.text) + " at " + ed.cursorPosition);
        findChild(menu, "md-slash-check").triggered();
        tryVerify(() => !menu.opened);
        compare(ed.text, "wor/ds\n- [ ] ", "the pick replaced the /");
        tryVerify(() => !menu.visible);
        ed.text = "words\n\n";
        ed.forceActiveFocus();
        ed.cursorPosition = ed.length;
        const before = ed.text;
        keyClick(Qt.Key_Slash);
        tryVerify(() => menu.opened, 1000, JSON.stringify(ed.text) + " " + ed.cursorPosition + " " + ed.activeFocus);
        keyClick(Qt.Key_Escape);
        tryVerify(() => !menu.opened);
        wait(50);
        compare(ed.text, before, "Esc took the / back");
    }

    // a-04 .. a-10, b-25: what is drawn over the note's source.
    function test_the_note_draws_as_the_sheet() {
        const task = mkTask("Pill probe");
        tryVerify(() => AppController.taskKeys[task] !== undefined, 2000, "no key for the task");
        const key = AppController.taskKeys[task];
        const body = "- a list item long enough to wrap over the end of the line and onto the next one, so it has to wrap, and then it goes on for a good while longer still\n"
                   + "\nkey `login` in #" + task + " and [RFC](https://x.test)\n"
                   + "\n```go\nwait := 1\n```\n"
                   + "\n![Retries](attachments/none.png)\n";
        const nv = richNotes(note("Look probe", body));
        const ed = findChild(nv, "notesEditor");
        tryVerify(() => decorated(nv, "md-live-bullet").length === 1, 2000, "no bullet");
        // The wrapped line starts under the item's text, not under the bullet.
        // The note may open under its title: the item is where its text is.
        const item = ed.text.indexOf("- a list");
        tryVerify(() => ed.positionToRectangle(item + 140).y > ed.positionToRectangle(item + 2).y, 2000, "the item did not wrap: " + item + " " + ed.positionToRectangle(item + 140) + " " + ed.positionToRectangle(item + 2) + " w " + ed.width);
        const wrapped = ed.positionToRectangle(item + 140);
        let lineStart = item + 140;
        while (lineStart > item + 2 && Math.abs(ed.positionToRectangle(lineStart - 1).y - wrapped.y) < 1) lineStart--;
        tryVerify(() => Math.round(ed.positionToRectangle(lineStart).x) === Math.round(ed.positionToRectangle(item + 2).x), 2000,
                "no hanging indent: marker at " + ed.positionToRectangle(item).x + ", wrap at " + lineStart);
        const bullet = decorated(nv, "md-live-bullet")[0];
        verify(bullet.x + bullet.width <= ed.positionToRectangle(item + 2).x, "the bullet is not in the gutter");
        // `code` in a pill, the task as ring and key, a link's dim line.
        const pills = decorated(nv, "md-live-inline-code");
        compare(pills.length, 1);
        verify(pills[0].width > 0);
        const ticket = decorated(nv, "md-live-ticket");
        compare(ticket.length, 1, "the task reference is not drawn");
        let keyShown = false;
        const walk = (it) => { if (it.text === key) keyShown = true; for (const k of (it.children || [])) walk(k); };
        walk(ticket[0]);
        verify(keyShown, "the pill does not show the key " + key);
        const lines = decorated(nv, "md-live-link-underline");
        compare(lines.length, 1);
        compare(String(lines[0].color), String(Theme.linkUnderline));
        // The code block's header: its language, "copy" in lower case, a rule.
        const copy = decorated(nv, "md-live-code-copy");
        compare(copy.length, 1);
        compare(copy[0].text, I18n.t("notes.code.copy"));
        compare(copy[0].text, copy[0].text.toLowerCase());
        compare(decorated(nv, "md-live-code-rule").length, 1, "no hairline under the code's header");
        // The picture's box over its room, the alt text under it.
        const fig = decorated(nv, "md-live-image");
        compare(fig.length, 1, "no picture drawn");
        const imgLine = ed.positionToRectangle(ed.text.indexOf("!["));
        verify(fig[0].height > 0 && fig[0].y + fig[0].height <= imgLine.y, "the picture is not above its caption");
    }

    // a-02 (task document): its text at 1.65.
    function test_task_document_reads_at_one_point_six_five() {
        const id = mkTask("Doc leading", "first line\nsecond line");
        const doc = createTemporaryQmlObject('import TodoCpp; TaskDocument { anchors.fill: parent }', host);
        doc.open(id);
        const field = findChild(findChild(doc, "task-doc-body"), "md-block-field");
        tryVerify(() => field.visible, 1000);
        const want = Math.round(field.font.pixelSize * 1.65);
        tryVerify(() => Math.abs(field.positionToRectangle(11).y - field.positionToRectangle(0).y - want) <= 1, 2000,
                  "lines " + (field.positionToRectangle(11).y - field.positionToRectangle(0).y) + " apart, want " + want);
        doc.close();
    }
}
