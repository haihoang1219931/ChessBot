import QtQuick 2.12
import QtQuick.Layouts 1.12

FocusScope {
    id: root
    width: 640
    height: 480
    // 3. FocusScope needs focus: true to accept focus from StackView
    focus: true

    signal itemSelected(string rank, string score)

    onItemSelected: (rank, score) => {
        console.log("SELECTED: " + rank + " with score: " + score);
    }

    // This ensures that when the FocusScope gets focus, it passes it to rankList
    onActiveFocusChanged: {
        if (activeFocus) {
            rankList.forceActiveFocus()
        }
    }

    property var rankData: [
        { name: "Grandmaster", scores: [2700, 2500, 2300] },
        { name: "Master",      scores: [2100, 1900, 1700] },
        { name: "Advanced",    scores: [1500, 1300, 1100] }
    ]

    Rectangle {
        anchors.fill: parent
        color: "#050505"
        RowLayout {
            anchors.fill: parent
            spacing: 0

            ListView {
                id: rankList
                Layout.fillWidth: true; Layout.fillHeight: true
                model: root.rankData
                focus: true // Default focus child
                KeyNavigation.right: scoreList
                clip: true
                highlightFollowsCurrentItem: true

                delegate: Item {
                    width: rankList.width; height: 70
                    readonly property bool isSelected: ListView.isCurrentItem

                    // Selection Bars
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

            Rectangle { Layout.preferredWidth: 2; Layout.fillHeight: true; color: "#222" }

            ListView {
                id: scoreList
                Layout.fillWidth: true; Layout.fillHeight: true
                model: root.rankData[rankList.currentIndex].scores
                KeyNavigation.left: rankList
                clip: true

                Keys.onReturnPressed: root.itemSelected(root.rankData[rankList.currentIndex].name, model[currentIndex])
                Keys.onEnterPressed: root.itemSelected(root.rankData[rankList.currentIndex].name, model[currentIndex])

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
}
