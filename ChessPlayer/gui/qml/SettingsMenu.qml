import QtQuick 2.0
import QtQuick.Layouts 1.12

FocusScope {
    id: root
    width: 800
    height: 480
    signal exitPressed()
    signal selectCalibration(string calibType)
    
    focus: true
    
    onActiveFocusChanged: {
        if (activeFocus) {
            settingsListView.forceActiveFocus()
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "#050505"

        ListModel {
            id: settingsModel
            ListElement { name: "Calibrate Camera Trapezoid"; type: "camera" }
//            ListElement { name: "Calibrate Chess Board & Drop Zones"; type: "chessboard" }
            ListElement { name: "Send test command"; type: "command" }
            ListElement { name: "Test Bot Vision"; type: "vision" }
            ListElement { name: "Back"; type: "back" }
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 20

            Text {
                text: "CALIBRATION SETTINGS"
                color: "white"
                font.pixelSize: 32
                font.family: "Orbitron"
                font.bold: true
                Layout.alignment: Qt.AlignHCenter
            }

            ListView {
                id: settingsListView
                Layout.fillWidth: true
                Layout.fillHeight: true
                model: settingsModel
                focus: true

                highlight: Rectangle {
                    color: "skyblue"
                    radius: 5
                    opacity: 0.3
                    Behavior on y { SpringAnimation { spring: 3; damping: 0.2 } }
                }

                delegate: Item {
                    width: parent.width
                    height: 70

                    property bool isSelected: ListView.isCurrentItem

                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: 5
                        color: isSelected ? "#22FFFFFF" : "transparent"
                        radius: 4
                        border.color: isSelected ? "#4488ff" : "transparent"
                        border.width: 1

                        Text {
                            anchors.fill: parent
                            anchors.leftMargin: 20
                            anchors.verticalCenter: parent.verticalCenter
                            text: model.name
                            color: isSelected ? "white" : "#AAAAAA"
                            font.pixelSize: 18
                            font.family: "Orbitron"
                            verticalAlignment: Text.AlignVCenter
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: settingsListView.currentIndex = index
                        }
                    }
                }

                Keys.onReturnPressed: {
                    var currentItem = settingsModel.get(currentIndex);
                    if (currentItem.type === "back") {
                        exitPressed();
                    } else {
                        selectCalibration(currentItem.type);
                    }
                }

                Keys.onEscapePressed: {
                    exitPressed();
                }
            }
        }
    }
}
