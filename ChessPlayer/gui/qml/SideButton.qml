import QtQuick 2.0
import QtQuick.Shapes 1.12

Item {
    id: root
    width: 150; height: 180

    property string sideName: ""
    property string iconText: "" // The unicode character for the icon
    property bool isSelected: false
    signal clicked()

    Column {
        anchors.fill: parent
        spacing: 15

        Item {
            width: 120; height: 120
            anchors.horizontalCenter: parent.horizontalCenter

            // 1. The Main Circle
            Rectangle {
                anchors.fill: parent
                radius: width / 2
                color: root.isSelected ? "#CCF0FF" : "#444444" // Darker when not selected

                // --- ICON FONT INSTEAD OF IMAGE ---
                Text {
                    anchors.centerIn: parent
                    text: root.iconText
                    font.family: iconFont.name
                    font.pixelSize: 70
                    // Change icon color based on selection or side
//                    color: root.isSelected ? "black" : "white"
                }
            }

            // 2. The Four Corner Brackets (Disconnected)
            Shape {
                anchors.fill: parent
                anchors.margins: -10
                visible: root.isSelected

                // 2. The Four Corner Brackets using PathArc
                Shape {
                    anchors.fill: parent
                    visible: root.isSelected

                    // Top-Left Corner
                    ShapePath {
                        strokeColor: "#0055ff"; strokeWidth: 4; fillColor: "transparent"
                        capStyle: ShapePath.RoundCap
                        startX: 0; startY: 30 // Start point of the arc
                        PathArc { x: 30; y: 00; radiusX: 30; radiusY: 30} // End point
                    }

                    // Top-Right Corner
                    ShapePath {
                        strokeColor: "#0055ff"; strokeWidth: 4; fillColor: "transparent"
                        capStyle: ShapePath.RoundCap
                        startX: 110; startY: 0
                        PathArc { x: 140; y: 30; radiusX: 30; radiusY: 30 }
                    }

                    // Bottom-Left Corner
                    ShapePath {
                        strokeColor: "#0055ff"; strokeWidth: 4; fillColor: "transparent"
                        capStyle: ShapePath.RoundCap
                        startX: 30; startY: 140
                        PathArc { x: 0; y: 110; radiusX: 30; radiusY: 30 }
                    }

                    // Bottom-Right Corner
                    ShapePath {
                        strokeColor: "#0055ff"; strokeWidth: 4; fillColor: "transparent"
                        capStyle: ShapePath.RoundCap
                        startX: 140; startY: 110
                        PathArc { x: 110; y: 140; radiusX: 30; radiusY: 30 }
                    }
                }
            }
        }

        // 3. Selection Text
        Text {
            text: (root.isSelected ? "▷ " : "") + root.sideName + (root.isSelected ? " ◁" : "")
            color: root.isSelected ? "#0055ff" : "white"
            font.pixelSize: 22
            font.bold: true
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }

    MouseArea { anchors.fill: parent; onClicked: root.clicked() }

    FontLoader {
        id: iconFont
        source: "qrc:/fonts/chess_merida_unicode.ttf"
        onStatusChanged: if (status == FontLoader.Ready) console.log("Loaded font name:", name)
    }
}
