// CI of a merge / pull request in the task document (APP-318): hidden for a
// task with no pipeline; the jobs and the log arrive by signal and are kept
// only while the panel shows that task.
import QtQuick
import QtTest
import TodoCpp

TestCase {
    id: tc
    name: "TaskCiPanel"
    when: windowShown
    visible: true
    width: 400
    height: 600

    Item { id: host; anchors.fill: parent }

    function test_hidden_without_a_pipeline_and_fills_from_signals() {
        const p = createTemporaryQmlObject('import TodoCpp; TaskCiPanel { width: 360 }', host);
        verify(p !== null);
        p.taskId = "no-such-task";
        verify(!p.visible, "no pipeline, no panel");
        // An answer for another task is not this panel's.
        AppController.ciJobsLoaded("other", [{ id: "1", name: "x", hasLog: true }], "");
        compare(p.jobs.length, 0);
        AppController.ciJobsLoaded("no-such-task", [{ id: "55", name: "test", stage: "test", seconds: 61, url: "https://g/j/55", hasLog: true }], "");
        compare(p.jobs.length, 1);
        p.logJob = "55";
        AppController.ciLogLoaded("no-such-task", "55", "ok\nFAILED: 1 test", "");
        compare(p.logText, "ok\nFAILED: 1 test");
        verify(AppController.isCiErrorLine("FAILED: 1 test"));
        // Another task: nothing carried over.
        p.taskId = "another";
        compare(p.jobs.length, 0);
        compare(p.logText, "");
    }
}
