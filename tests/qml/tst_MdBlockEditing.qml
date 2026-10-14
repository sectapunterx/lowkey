// The block editor's way from the drawn note into its source (notes bugs,
// 2026-10-14): a click on a table or a code block opens it, the caret lands
// where the click was, and a selection can run across blocks (Shift+click,
// Shift+Down at a block's edge, a second Ctrl+A) so more than one block can
// be copied.
import QtQuick
import QtTest
import TodoCpp

TestCase {
    id: tc
    name: "MdBlockEditing"
    when: windowShown
    visible: true
    width: 900
    height: 700

    Item { id: host; anchors.fill: parent }

    readonly property string sample: "# Title\n\nFirst paragraph with **bold** words.\n\n"
                                     + "| Name | Count |\n|---|---|\n| alpha | 1 |\n| beta | 22 |\n\n"
                                     + "```js\nfunction hello() {}\n```\n\nLast paragraph.\n"

    function make() {
        const ed = createTemporaryQmlObject('import TodoCpp; MdBlockEditor { anchors.fill: parent }', host);
        ed.text = tc.sample;
        wait(50);
        return ed;
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

    // Bug 1: the table's Flickable took the press; nothing opened.
    function test_a_click_on_a_table_opens_it() {
        const ed = make();
        const table = find(ed, "mdTable");
        verify(table !== null, "no table drawn");
        mouseClick(table, 20, 10);
        tryVerify(() => ed.editing, 1000, "the table did not open");
        verify(ed.editor.text.indexOf("| alpha | 1 |") >= 0, ed.editor.text);
    }

    function test_a_click_on_a_code_block_opens_it() {
        const ed = make();
        const code = find(ed, "mdCodeBody");
        verify(code !== null, "no code drawn");
        mouseClick(code, 10, 5);
        tryVerify(() => ed.editing, 1000, "the code block did not open");
        verify(ed.editor.text.indexOf("```js") === 0, ed.editor.text);
    }

    // Bug 3: the caret went to the end of the block wherever the click was.
    function test_the_caret_goes_where_the_click_was() {
        const ed = make();
        const t = "First paragraph with **bold** words.";
        // "bo|ld": the drawn text has no asterisks.
        const plain = "First paragraph with bold words.";
        compare(ed._offsetFor({ plain: plain, pos: plain.indexOf("bold") + 2 }, t), t.indexOf("bold") + 2);
        // A cell: the text around the click names the row.
        const table = "| Name | Count |\n|---|---|\n| alpha | 1 |\n| beta | 22 |";
        compare(ed._offsetFor({ plain: "beta", pos: 2 }, table), table.indexOf("beta") + 2);
        compare(ed._offsetFor(null, t), -1);
        // A cell knows its row and column: "22" at its start, "1" (in more
        // than one cell) at its end — the caret went to the table's start
        // and Backspace there joined the table to the block above.
        compare(ed._offsetFor({ plain: "22", pos: 0, cell: 5, columns: 2 }, table), table.indexOf("22"));
        compare(ed._offsetFor({ plain: "1", pos: 1, cell: 3, columns: 2 }, table), table.indexOf("| 1 |") + 3);
        compare(ed._offsetFor({ plain: "Name", pos: 0, cell: 0, columns: 2 }, table), 2);
        // Near the start of a text the piece around the click still counts.
        compare(ed._offsetFor({ plain: "Last paragraph.", pos: 0 }, "x\n\nLast paragraph."), 3);
        ed.editLine(2, false, { plain: plain, pos: plain.indexOf("words") });
        compare(ed.editor.cursorPosition, t.indexOf("words"));
    }

    // Bug 2: nothing past one block could be selected or copied.
    function test_a_second_ctrl_a_selects_the_whole_note() {
        const ed = make();
        ed.editLine(2, false);
        tryVerify(() => ed.editing);
        keySequence(StandardKey.SelectAll);
        compare(ed.editor.selectedText, "First paragraph with **bold** words.");
        keySequence(StandardKey.SelectAll);
        compare(ed.editor.text, tc.sample.replace(/\n+$/, ""));
        compare(ed.editor.selectionStart, 0);
        compare(ed.editor.selectionEnd, ed.editor.length);
        // Leaving puts every block back as it was.
        keyClick(Qt.Key_Escape);
        compare(ed.rawText, tc.sample);
    }

    function test_shift_down_at_the_edge_runs_into_the_next_block() {
        const ed = make();
        ed.editLine(2, false);
        ed.editor.cursorPosition = ed.editor.text.indexOf("words");
        keyClick(Qt.Key_Down, Qt.ShiftModifier);
        verify(ed.editor.text.indexOf("| beta | 22 |") > 0, "the table did not join the field");
        compare(ed.editor.selectedText, "words.\n\n| Name | Count |\n|---|---|\n| alpha | 1 |\n| beta | 22 |");
        keyClick(Qt.Key_Escape);
        compare(ed.rawText, tc.sample, "a selection is not an edit");
    }

    function test_shift_click_on_another_block_selects_up_to_it() {
        const ed = make();
        ed.editLine(2, true);
        tryVerify(() => ed.editing);
        const lastRow = ed.document.rowForLine(16);
        verify(ed.extendToRow(lastRow, { plain: "Last paragraph.", pos: 4 }));
        verify(ed.editor.selectedText.indexOf("First paragraph") === 0, ed.editor.selectedText);
        verify(ed.editor.selectedText.endsWith("Last"), ed.editor.selectedText);
    }

    // Backspace at a block's start joins it to the block above, one line
    // break at a time, as in any text; Delete at the end, the one below.
    function test_backspace_and_delete_cross_the_block_edge() {
        const ed = createTemporaryQmlObject('import TodoCpp; MdBlockEditor { anchors.fill: parent; noteType: true }', host);
        let tail = "";
        for (let i = 0; i < 40; i++) tail += "\nfiller " + i + "\n";
        ed.text = "one\n\ntwo\n\nthree\n" + tail;
        wait(50);
        ed.editLine(2, true);
        compare(ed.editor.text, "two");
        const y0 = ed.editor.y;
        keyClick(Qt.Key_Backspace);
        compare(ed.editor.text, "one\ntwo");
        compare(ed.editor.cursorPosition, 4);
        // The field stays where the blocks are; it fell to the end of the
        // note when the rows were rebuilt under it (after the commit timer).
        wait(600);
        verify(ed.editor.y <= y0, "the field moved to " + ed.editor.y + " from " + y0);
        ed.editor.cursorPosition = ed.editor.length;
        keyClick(Qt.Key_Delete);
        compare(ed.editor.text, "one\ntwo\nthree");
        keyClick(Qt.Key_Escape);
        compare(ed.rawText, "one\ntwo\nthree\n" + tail);
    }

    // A join rebuilds the rows under the open field: it looked its row up
    // with itemAtIndex(), kept one that was gone and fell to the end of the
    // note, so the text being typed was out of sight.
    function test_the_field_stays_on_its_blocks_after_a_join() {
        const ed = createTemporaryQmlObject('import TodoCpp; MdBlockEditor { anchors.fill: parent; noteType: true }', host);
        ed.text = "# Title\n\nThe opening paragraph has **bold**.\nIt continues.\n\nSecond paragraph, one line.\n\n## Lists\n\n- one\n- two\n";
        wait(100);
        ed.editLine(5, true);
        wait(100);
        keyClick(Qt.Key_Backspace);
        const y = ed.editor.y;
        wait(600);
        compare(ed.editor.y, y, "the field left its blocks after the commit");
    }

    // "1. #": an empty heading in a list item draws its marker and opens its
    // own line, not the "$$" of a math block above it.
    function test_an_empty_heading_in_a_list_opens_its_own_line() {
        const ed = createTemporaryQmlObject('import TodoCpp; MdBlockEditor { anchors.fill: parent }', host);
        ed.text = "para\n\n$$\nE = mc^2\n$$\n\n1. #\n2. two\n";
        wait(50);
        const marker = find(ed, "mdHeadingMarker");
        verify(marker !== null && marker.visible, "the item's marker is not drawn");
        ed.editLine(6, false);
        compare(ed.editor.text, "1. #");
    }
}
