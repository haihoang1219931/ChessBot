import QtQuick 2.0
Item {
    id: root
    property var controller

    function pieceText(code) {
        switch (code) {
        case "wK": return "\u2654"
        case "wQ": return "\u2655"
        case "wR": return "\u2656"
        case "wB": return "\u2657"
        case "wN": return "\u2658"
        case "wP": return "\u2659"
        case "bK": return "\u265A"
        case "bQ": return "\u265B"
        case "bR": return "\u265C"
        case "bB": return "\u265D"
        case "bN": return "\u265E"
        case "bP": return "\u265F"
        default: return ""
        }
    }

    function pieceColor(code) {
        if (!code || code.length < 1) return "transparent"
        return code.charAt(0) === "w" ? "#ffffff" : "#111111"
    }

    Grid {
        id: chessGrid
        anchors.centerIn: parent
        rows: 8
        columns: 8

        property real boardSize: Math.min(parent.width - 40, parent.height - 40)
        property real tileSize: boardSize / 8

        Repeater {
            model: 64

            Rectangle {
                property int boardIndex: controller && controller.playerColor === 1 ? (63 - index) : index

                width: chessGrid.tileSize
                height: chessGrid.tileSize

                color: {
                    var rank = Math.floor(index / 8)
                    var file = index % 8
                    var light = ((rank + file) % 2) === 0
                    if (!controller) return light ? "#f2d9b0" : "#8a5b34"
                    if (controller.selectedSquare === boardIndex) return "#d35400"
                    if (controller.checkedKingSquare === boardIndex) return "#bb1f1f"
                    if (controller.isValidDestination(boardIndex)) return "#2e8b57"
                    return light ? "#f2d9b0" : "#8a5b34"
                }

                border.width: 1
                border.color: "#4f2f17"

                Text {
                    anchors.centerIn: parent
                    text: pieceText(controller && controller.board ? controller.board[boardIndex] : "")
                    color: pieceColor(controller && controller.board ? controller.board[boardIndex] : "")
                    font.pixelSize: parent.width * 0.55
                    font.bold: true
                    font.family: "Times New Roman"
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: if (controller) controller.clickSquare(boardIndex)
                }
            }
        }
    }
}
