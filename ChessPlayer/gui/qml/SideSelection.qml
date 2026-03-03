import QtQuick 2.12

// Use FocusScope to trap focus inside this component
FocusScope {
    id: root
    width: 640; height: 480
    focus: true // Signals that this scope wants focus

    signal sideConfirmed(string side)
    signal goback()
    // Ensure focus is grabbed immediately when the component loads
    Component.onCompleted: whiteItem.forceActiveFocus()

    Rectangle {
        anchors.fill: parent
        color: "#050505"

        Column {
            anchors.centerIn: parent
            spacing: 30

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Please choose White\nor Black"
                color: "white"
                font.pixelSize: 22; font.bold: true
                horizontalAlignment: Text.AlignHCenter
            }

            Row {
                spacing: 60
                anchors.horizontalCenter: parent.horizontalCenter

                SideButton {
                    id: whiteItem
                    sideName: "White"
                    iconText: "\u2654" // White King Unicode
                    KeyNavigation.right: blackItem
                    Keys.onReturnPressed: root.sideConfirmed("White")
                    Keys.onEscapePressed: root.goback()
                    isSelected: activeFocus
                }

                SideButton {
                    id: blackItem
                    sideName: "Black"
                    iconText: "\u265A" // Black King Unicode
                    KeyNavigation.right: whiteItem
                    Keys.onReturnPressed: root.sideConfirmed("Black")
                    Keys.onEscapePressed: root.goback()
                    isSelected: activeFocus
                }
            }
        }
    }
    onSideConfirmed: console.log("Confirmed: " + side)
}
