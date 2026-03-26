import QtQuick 2.0
import QtQuick.Layouts 1.12
import QtGraphicalEffects 1.0
FocusScope {
    id: root
    width: 640
    height: 480
    signal enterItem(var item)
    // 3. FocusScope needs focus: true to accept focus from StackView
    focus: true

    // This ensures that when the FocusScope gets focus, it passes it to rankList
    onActiveFocusChanged: {
        if (activeFocus) {
            menuListView.forceActiveFocus()
        }
    }
    Rectangle {
        anchors.fill: parent
        color: "#050505"
        ListModel {
            id: menuModel
            ListElement { name: "Challenge AI" }
            ListElement { name: "Settings" }
        }
        RowLayout {
            anchors.fill: parent
            spacing: 0
            StackLayout {
                width: 440
                height: 400
                currentIndex: menuListView.currentIndex
                MenuBotIcon{ }
                MenuSettingIcon{ }
            }

            ListView {
                id: menuListView
                width: 200; height: 200
                model: menuModel
                focus: true // Essential for keyboard navigation

                // The visual highlight for the selected item
                highlight: Rectangle {
                    color: "skyblue"
                    radius: 5
                    opacity: 0.3
                    Behavior on y { SpringAnimation { spring: 3; damping: 0.2 } }
                }

                delegate: Item {
                    width: parent.width
                    height: 60

                    // Highlight state handler
                    property bool isSelected: ListView.isCurrentItem

                    // Item Container
                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: 5
                        color: isSelected ? "#22FFFFFF" : "transparent"
                        radius: 4
                        border.color: isSelected ? "#4488ff" : "transparent"
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 20
                            spacing: 15


                            // Main Text
                            Text {
                                width: parent.width - 40
                                text: model.name
                                color: isSelected ? "white" : "#AAAAAA"
                                font.pixelSize: 22
                                font.family: "Orbitron" // Use a sci-fi font if available
                                font.letterSpacing: 1.5

                                // Text Glow Effect
                                layer.enabled: isSelected
                                layer.effect: Glow {
                                    samples: 15
                                    color: "#4488ff"
                                    transparentBorder: true
                                }
                            }

                            // Selection Indicator (The "Arrow")
                            Text {
                                text: "◀"
                                color: "#4488ff"
                                font.pixelSize: 24
                                visible: isSelected

                                // Simple pulse animation for selected arrow
                                SequentialAnimation on opacity {
                                    running: isSelected
                                    loops: Animation.Infinite
                                    NumberAnimation { from: 0.5; to: 1.0; duration: 800 }
                                    NumberAnimation { from: 1.0; to: 0.5; duration: 800 }
                                }
                            }
                        }
                    }
                }

                // Custom keyboard actions (e.g., pressing Enter)
                Keys.onReturnPressed: {
                    root.enterItem(currentIndex)
                    console.log("Selected:", menuModel.get(currentIndex).name)
                }
            }
        }
    }
}
