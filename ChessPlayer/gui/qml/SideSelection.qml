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
        color: "#0a0e14"

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

                // --- WHITE SIDE ---
                Item {
                    id: whiteItem
                    width: 140; height: 180
                    focus: true // Default item within the scope

                    KeyNavigation.right: blackItem
                    Keys.onReturnPressed: root.sideConfirmed("White")
                    Keys.onEscapePressed: root.goback()

                    // VISUALS
                    Column {
                        anchors.fill: parent
                        spacing: 15
                        Rectangle {
                            width: 110; height: 110; radius: 55
                            anchors.horizontalCenter: parent.horizontalCenter
                            color: whiteItem.activeFocus ? "#bde0fe" : "#2c3e50"

                            // Yellow brackets
                            Rectangle {
                                anchors.fill: parent; anchors.margins: -10
                                color: "transparent"; border.color: "yellow"; border.width: 3; radius: 60
                                visible: whiteItem.activeFocus
                            }
                            Text { anchors.centerIn: parent; text: "♔"; font.pixelSize: 60; color: "black" }
                        }
                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: whiteItem.activeFocus ? "▷ White ◁" : "White"
                            color: whiteItem.activeFocus ? "yellow" : "#7f8c8d"
                            font.pixelSize: 20; font.bold: true
                        }
                    }
                }

                // --- BLACK SIDE ---
                Item {
                    id: blackItem
                    width: 140; height: 180

                    KeyNavigation.left: whiteItem
                    Keys.onReturnPressed: root.sideConfirmed("Black")
                    Keys.onEscapePressed: root.goback()

                    Column {
                        anchors.fill: parent
                        spacing: 15
                        Rectangle {
                            width: 110; height: 110; radius: 55
                            anchors.horizontalCenter: parent.horizontalCenter
                            color: blackItem.activeFocus ? "#bde0fe" : "#2c3e50"

                            Rectangle {
                                anchors.fill: parent; anchors.margins: -10
                                color: "transparent"; border.color: "yellow"; border.width: 3; radius: 60
                                visible: blackItem.activeFocus
                            }
                            Text { anchors.centerIn: parent; text: "♚"; font.pixelSize: 60; color: "black" }
                        }
                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: blackItem.activeFocus ? "▷ Black ◁" : "Black"
                            color: blackItem.activeFocus ? "yellow" : "#7f8c8d"
                            font.pixelSize: 20; font.bold: true
                        }
                    }
                }
            }
        }
    }
    onSideConfirmed: console.log("Confirmed: " + side)
}
