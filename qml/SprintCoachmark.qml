import QtQuick
import TodoCpp

// The first Jira sprint end shown in the calendar (coach mark), under its
// "S 14 ends" mark: it comes from Jira, how many of the user's cards are in
// it, and that it moves nothing. For the active sprint it offers the search
// that lists them — the query goes into the search box, nothing more.
CoachMark {
    id: root
    markId: "sprint"

    // AppController.coachMark: {id, name, state, end, tasks}
    property var hint: ({})
    readonly property string _query: "sprint:current"
    readonly property bool _active: String(root.hint.state || "") === "active"
    readonly property string _end: root.hint.end ? I18n.fmtDate(new Date(root.hint.end), "longDay") : ""

    title: I18n.t("coach.sprint.title").arg(String(root.hint.name || "")).arg(root._end)
    body: I18n.t("coach.sprint.body").arg(Number(root.hint.tasks || 0))
    secondaryText: root._active ? I18n.t("coach.sprint.show").arg(root._query) : ""
    onPrimaryClicked: root.finish()
    onSecondaryClicked: {
        const w = root.hostWindow;
        root.finish(function () { if (w && w.showQuery) w.showQuery(root._query); });
    }
}
