import QtQuick 2.15
import QtQuick.Shapes 1.15

Item {
    width: 640
    height: 480
    // Left Side (Red Section with Diagonal Cut)
    Shape {
        anchors.fill: parent
        ShapePath {
            fillGradient: LinearGradient {
                x1: 0; y1: 0; x2: width; y2: height
                GradientStop { position: 0.0; color: "#b32400" }
                GradientStop { position: 1.0; color: "#ff4d00" }
            }
            // Defines the diagonal trapezoid shape
            startX: 0; startY: 0
            PathLine { x: width * 0.65; y: 0 }    // Top edge
            PathLine { x: width * 0.45; y: height } // Bottom edge (creates slash)
            PathLine { x: 0; y: height }
            PathLine { x: 0; y: 0 }
        }

        Column {
            x: 50; anchors.verticalCenter: parent
            Text { text: "Julie"; color: "white"; font.pixelSize: 48 }
            Text { text: "05:39"; color: "white"; font.pixelSize: 72; font.bold: true }
        }
    }

    // Right Side (Blue Section)
    Rectangle {
        z: -1 // Place behind the red shape
        anchors.fill: parent
        color: "#001a33"

        Text {
            anchors.right: parent.right
            anchors.rightMargin: 100
            anchors.verticalCenter: parent
            text: "02:53"
            color: "#66ccff"
            font.pixelSize: 64
        }
    }
    // Current Level Display at the Top
    Text {
        id: levelDisplay
        text: "level 1 (200)"
        color: "white"
        font.pixelSize: 24
        font.family: "Arial" // Use a clean, sans-serif font

        // Positioning
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 20

        // Optional: Adding the small "level" icon/bar seen in the image
        Rectangle {
            width: 15; height: 5
            color: "white"
            anchors.right: parent.left
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
