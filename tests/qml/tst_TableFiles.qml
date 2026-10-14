// Tables and text files (APP-315): a .csv pasted into a note is its table,
// the file linked under it; a .csv / .log attached opens in the app.
import QtQuick
import QtTest
import TodoCpp

TestCase {
    id: tc
    name: "TableFiles"
    when: windowShown
    visible: true
    width: 1100
    height: 760

    Item { id: host; anchors.fill: parent }
    readonly property url csvFile: Qt.resolvedUrl("data/table.csv")
    readonly property url logFile: Qt.resolvedUrl("data/run.log")

    function test_a_csv_pasted_into_text_is_its_table() {
        const added = AppController.importAttachments([tc.csvFile], true);
        compare(added.length, 1);
        const ref = String(added[0].ref);
        verify(ref.startsWith("| name | city | note |\n| --- | --- | --- |\n| Алёна | Москва | line one line two |"), ref);
        verify(ref.indexOf("| Bob | Paris | says \"hi\" |") > 0, ref);
        verify(ref.indexOf("](attachments/") > 0, "the file is not linked under the table: " + ref);
    }

    function test_a_csv_opens_in_the_app() {
        const id = AppController.importAttachments([tc.csvFile], false)[0].id;
        compare(AppController.openAttachment(id, false), "view");
        const p = AppController.attachmentPreview(id);
        compare(p.kind, "table");
        compare(p.sheets.length, 1);
        compare(p.sheets[0].rows.length, 3);
        compare(p.sheets[0].columns, 3);
        compare(p.sheets[0].rows[1][2], "line one\nline two");

        const chips = createTemporaryQmlObject('import TodoCpp; AttachmentChips { model: [] }', host);
        chips.open(id, "table.csv");
        const viewer = chips._viewer;
        verify(viewer !== null, "no viewer");
        tryCompare(viewer, "opened", true);
        const grid = findChild(viewer.contentItem, "file-viewer-grid");
        verify(grid !== null);
        tryCompare(grid, "count", 3);
        viewer.close();
    }

    function test_a_log_opens_as_a_log() {
        const id = AppController.importAttachments([tc.logFile], false)[0].id;
        compare(AppController.openAttachment(id, false), "view");
        const p = AppController.attachmentPreview(id);
        compare(p.kind, "text");
        compare(p.language, "log");
        verify(String(p.text).indexOf("ERROR src/a.cc:12") > 0);
    }
}
