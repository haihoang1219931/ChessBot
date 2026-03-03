import QtQuick 2.12
import QtQuick.Shapes 1.12

Rectangle {
    id: root
    width: 640
    height: 480
    color: "black"
    signal goback()
    Keys.onEscapePressed: root.goback()
    Column {
        anchors.fill: parent

        // --- TOP OVERLAY SECTION ---
        Rectangle {
            id: topBar
            width: parent.width
            height: 60 // Defined height for the top bar
            color: "transparent"

            Row {
                anchors.left: parent.left
                anchors.leftMargin: 20
                spacing: 8

                Rectangle {
                    width: 18; height: 6
                    color: "white"
                    anchors.verticalCenter: parent.verticalCenter
                }

                Text {
                    text: "level 1 (200)"
                    color: "white"
                    font.pixelSize: 24
                    font.weight: Font.DemiBold
                }
            }
        }

        // --- SHAPE LAYER SECTION (Main Timer Area) ---
        Item {
            width: parent.width
            height: parent.height - topBar.height // Take up remaining space

            // Blue Background (Right)
            Rectangle {
                anchors.fill: parent
                color: "#001e3e"

                Text {
                    anchors.right: parent.right
                    anchors.rightMargin: 50
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 50
                    text: "02:53"
                    color: "#66ccff"
                    font.pixelSize: 80
                    font.bold: true
                }
            }

            // Red/Orange Shape with Slash (Left)
            Shape {
                anchors.fill: parent
                smooth: true

                ShapePath {
                    strokeWidth: 0
                    fillGradient: LinearGradient {
                        x1: 0; y1: 0; x2: root.width; y2: root.height
                        GradientStop { position: 0.0; color: "#9a1a00" }
                        GradientStop { position: 0.6; color: "#ff4d00" }
                    }

                    startX: 0; startY: 0
                    PathLine { x: root.width * 0.62; y: 0 }
                    PathLine { x: root.width * 0.42; y: root.height }
                    PathLine { x: 0; y: root.height }
                    PathLine { x: 0; y: 0 }
                }

                Column {
                    x: 60
                    spacing: -5
                    Text { text: "Julie"; color: "white"; font.pixelSize: 52 }
                    Text { text: "05:39"; color: "white"; font.pixelSize: 96; font.bold: true }
                }
            }
        }
    }
}
