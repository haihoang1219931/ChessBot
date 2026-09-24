import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

// Use FocusScope to trap focus inside this component
FocusScope {
    id: root
    width: 800; height: 480
    focus: true // Signals that this scope wants focus

    signal itemSelected(string expireTime)
    signal exitPressed()
    property var listTimeModel: ["30 mins","10 mins","No limit"]
    onItemSelected: (expireTime) => {
        console.log("Selected expireTime: " + expireTime);
    }

    Rectangle {
        anchors.fill: parent
        color: "#050505"

        ColumnLayout {
            anchors.centerIn: parent
            spacing: 30

            Text {
                text: "Please choose time out"
                color: "white"
                font.pixelSize: 22; font.bold: true
                Layout.alignment: Qt.AlignHCenter
            }

            RowLayout {
                spacing: 60
                Layout.alignment: Qt.AlignHCenter
                ListView {
                    id: listTime
                    Layout.preferredWidth: root.width * 2/3
                    Layout.preferredHeight: 210
                    Layout.alignment: Qt.AlignVCenter
                    orientation: ListView.Vertical
                    verticalLayoutDirection: ListView.TopToBottom
                    model: root.listTimeModel
                    focus: true // Default focus child
                    clip: true
                    highlightFollowsCurrentItem: true
                    Keys.onReturnPressed: root.itemSelected(root.listTimeModel[currentIndex]);
                    Keys.onSpacePressed: root.itemSelected(root.listTimeModel[currentIndex]);
                    Keys.onEscapePressed: root.exitPressed();
                    delegate: Item {
                        width: listTime.width; height: 70
                        readonly property bool isSelected: ListView.isCurrentItem

                        // Selection Bars
                        Rectangle {
                            visible: isSelected
                            width: parent.width; height: 2; anchors.top: parent.top
                            gradient: Gradient {
                                orientation: Gradient.Horizontal
                                GradientStop { position: 0.0; color: "transparent" }
                                GradientStop { position: 0.5; color: "#0055ff" }
                                GradientStop { position: 1.0; color: "transparent" }
                            }
                        }
                        Rectangle {
                            visible: isSelected
                            width: parent.width; height: 2; anchors.bottom: parent.bottom
                            gradient: Gradient {
                                orientation: Gradient.Horizontal
                                GradientStop { position: 0.0; color: "transparent" }
                                GradientStop { position: 0.5; color: "#0055ff" }
                                GradientStop { position: 1.0; color: "transparent" }
                            }
                        }

                        Text {
                            anchors.centerIn: parent
                            text: (isSelected ? "> " : "") + modelData
                            color: isSelected ? "#0055ff" : "white"
                            font.pixelSize: 28; font.bold: isSelected
                        }
                    }
                }
            }
        }
    }
}
