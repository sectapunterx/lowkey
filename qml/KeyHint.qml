import QtQuick
import TodoCpp

// A key beside the action it runs (APP-259): "d", "Shift S", "g b",
// "Ctrl K". Notation (keymap.md): a lowercase letter is the key without
// Shift, Shift is spelled out, a two-key sequence is written with a space.
// On-screen hints (the bar at the bottom, the selected card, a lens tab)
// follow Style.keyHints; a menu shows its keys always (`always: true`).
Text {
    id: root

    property string keys: ""
    property bool always: false

    visible: keys.length > 0 && (always || Style.keyHints)
    text: keys
    color: Theme.textDim
    // A key cap in the text face (APP-314): small mono letters beside every
    // row read as specks on a 2K screen; a framed key reads as a key.
    font.family: Theme.fontUi
    font.pixelSize: Theme.fsXs
    verticalAlignment: Text.AlignVCenter
    leftPadding: Theme.px(5)
    rightPadding: Theme.px(5)
    topPadding: 1
    bottomPadding: 1
    Accessible.ignored: true

    Rectangle {
        z: -1
        anchors.fill: parent
        radius: 4
        color: "transparent"
        border.width: 1
        border.color: Theme.borderStrong
    }
}
