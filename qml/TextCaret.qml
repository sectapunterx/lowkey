import QtQuick
import TodoCpp

// The text caret of the long text fields (a task's notes, a note): two
// pixels in the cursor colour, blinking while its field has the keyboard.
// The Basic style's one-pixel caret was lost on a dark page, and "my notes"
// gave no sign that they could be typed into (dogfood 2026-10-16).
//
//   TextArea { cursorDelegate: TextCaret { field: myField } }
Rectangle {
    id: caret
    property Item field: parent
    // A drawn document that does not hold the keyboard still shows where
    // the text would go, dimmed and still.
    property bool ghost: false

    width: Theme.px(2)
    radius: width / 2
    color: Theme.focusRing
    readonly property bool _live: field !== null && field.activeFocus
    visible: _live || ghost
    opacity: _live ? (_on ? 1 : 0) : 0.45
    property bool _on: true
    Timer {
        running: caret._live
        repeat: true
        interval: 530
        onTriggered: caret._on = !caret._on
        onRunningChanged: caret._on = true
    }
    // Typing or moving shows it at once instead of mid-blink.
    Connections {
        target: caret.field
        ignoreUnknownSignals: true
        function onCursorPositionChanged() { caret._on = true; }
    }
}
