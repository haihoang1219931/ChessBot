import QtQuick 2.12
import QtQuick.Layouts 1.12

Rectangle {
    id: root
    width: 800; height: 480
    color: "#050505"

    // CUSTOM SIGNAL: Emitted when Enter is pressed on a score
    signal itemSelected(string rank, string score)

    // Log the selection to the console when the signal is triggered
    onItemSelected: (rank, score) => {
        console.log("SELECTED: " + rank + " with score: " + score)
    }

    property var rankData: [
        { name: "Grandmaster", scores: ["2700", "2500", "2300"] },
        { name: "Master",      scores: ["2100", "1900", "1700"] },
        { name: "Advanced",    scores: ["1500", "1300", "1100"] }
    ]

    RowLayout {
        anchors.fill: parent;
        anchors.horizontalCenter: parent.horizontalCenter;
        spacing: 0

        // --- LEFT LIST: Ranks ---
        ListView {
            id: rankList
            Layout.fillWidth: true; Layout.fillHeight: true
            model: root.rankData
            focus: true
            KeyNavigation.right: scoreList
            clip: true

            delegate: Item {
                width: rankList.width; height: 70
                readonly property bool isSelected: ListView.isCurrentItem

                Rectangle {
                    visible: isSelected
                    width: parent.width; height: 2; anchors.top: parent.top
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0.0; color: "transparent" }
                        GradientStop { position: 0.5; color: "#ffff00" }
                        GradientStop { position: 1.0; color: "transparent" }
                    }
                }
                Rectangle {
                    visible: isSelected
                    width: parent.width; height: 2; anchors.bottom: parent.bottom
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0.0; color: "transparent" }
                        GradientStop { position: 0.5; color: "#ffff00" }
                        GradientStop { position: 1.0; color: "transparent" }
                    }
                }

                Text {
                    anchors.centerIn: parent
                    text: (isSelected && rankList.activeFocus ? "> " : "") + modelData.name
                    color: isSelected ? "#ffff00" : "white"
                    font.pixelSize: 28; font.bold: isSelected
                }
            }
        }

        // VERTICAL DIVIDER
        Rectangle { Layout.preferredWidth: 2; Layout.fillHeight: true; color: "#222" }

        // --- RIGHT LIST: Scores ---
        ListView {
            id: scoreList
            Layout.fillWidth: true; Layout.fillHeight: true
            model: root.rankData[rankList.currentIndex].scores
            KeyNavigation.left: rankList
            clip: true

            // --- KEY EVENT: ENTER/RETURN ---
            Keys.onReturnPressed: {
                root.itemSelected(root.rankData[rankList.currentIndex].name, model[currentIndex])
            }
            Keys.onEnterPressed: {
                root.itemSelected(root.rankData[rankList.currentIndex].name, model[currentIndex])
            }

            delegate: Item {
                width: scoreList.width; height: 70
                readonly property bool isSelected: ListView.isCurrentItem && scoreList.activeFocus

                Rectangle {
                    visible: isSelected
                    width: parent.width; height: 2; anchors.top: parent.top
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0.0; color: "transparent" }
                        GradientStop { position: 0.5; color: "#ffff00" }
                        GradientStop { position: 1.0; color: "transparent" }
                    }
                }
                Rectangle {
                    visible: isSelected
                    width: parent.width; height: 2; anchors.bottom: parent.bottom
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0.0; color: "transparent" }
                        GradientStop { position: 0.5; color: "#ffff00" }
                        GradientStop { position: 1.0; color: "transparent" }
                    }
                }

                Text {
                    anchors.centerIn: parent
                    text: (isSelected ? "> " : "") + modelData
                    color: isSelected ? "#ffff00" : (scoreList.activeFocus ? "white" : "#666")
                    font.pixelSize: 28; font.bold: isSelected
                }
            }
        }
    }
}
