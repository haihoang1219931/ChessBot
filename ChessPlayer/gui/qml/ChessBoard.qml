import QtQuick 2.0
Item {
    id: root
    property var board
    property var playerColor
    property var selectedSquare
    property var checkedKingSquare
    property bool activeUserInput: false
    property int userInputIndex: 56
    property int userInputIndexStart:-1
    property int userInputIndexStop:-1

    function enableUserInput(enable) {
        if(!activeUserInput) {
            activeUserInput = enable;
            userInputIndex = 56;
            userInputIndexStart = -1
            userInputIndexStop = -1
        }
    }

    function updateUserInput(direction) {
        if(userInputIndex+direction>=0 && userInputIndex+direction<64)
            userInputIndex += direction;
    }
    function cancelUserSelection() {
        if(userInputIndexStop != -1) {
            userInputIndexStop = -1;
        } else if(userInputIndexStart != -1) {
            userInputIndex = userInputIndexStart;
            userInputIndexStart = -1;
        } else if(userInputIndexStop == -1 &&
                  userInputIndexStart == -1) {
            activeUserInput = false;
        }
    }

    function updateUserSelection() {
        if(activeUserInput) {
            if(userInputIndexStart == -1) {
                userInputIndexStart = userInputIndex;
            } else if(userInputIndexStop == -1 && userInputIndex!=userInputIndexStart) {
                userInputIndexStop = userInputIndex;
            }
            if(userInputIndexStop != -1 && userInputIndexStart != -1) {
                backend.playInputMove(userInputIndexStart,userInputIndexStop);
                activeUserInput = false;
            }
            console.log("move from "+userInputIndexStart+" to "+userInputIndexStop)
        }
    }

    function pieceText(code) {
        switch (code) {
        case "wK": return "\u265A"
        case "wQ": return "\u265B"
        case "wR": return "\u265C"
        case "wB": return "\u265D"
        case "wN": return "\u265E"
        case "wP": return "\u265F"
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
        return code.charAt(0) === "w" ? "gray" : "orange"
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
                property int boardIndex: playerColor === 1 ? (63 - index) : index

                width: chessGrid.tileSize
                height: chessGrid.tileSize

                color: {
                    var rank = Math.floor(index / 8)
                    var file = index % 8
                    var light = ((rank + file) % 2) === 0
                    if (selectedSquare === boardIndex) return "#d35400"
                    if (checkedKingSquare === boardIndex) return "#bb1f1f"
                    return light ? "white" : "black"
                }
                Rectangle {
                    width: chessGrid.tileSize
                    height: chessGrid.tileSize
                    color: "transparent"
                    border.width: 5
                    border.color: "steelblue"
                    visible: root.activeUserInput &&
                             index == root.userInputIndex
                }
                Rectangle {
                    width: chessGrid.tileSize
                    height: chessGrid.tileSize
                    color: "transparent"
                    border.width: 5
                    border.color: "red"
                    visible: root.activeUserInput &&
                             index == root.userInputIndexStart
                }
                Rectangle {
                    width: chessGrid.tileSize
                    height: chessGrid.tileSize
                    color: "transparent"
                    border.width: 5
                    border.color: "orange"
                    visible: root.activeUserInput &&
                             index == root.userInputIndexStop
                }
                Text {
                    anchors.centerIn: parent
                    text: pieceText(board ? board[boardIndex] : "")
                    color: pieceColor(board ? board[boardIndex] : "")
                    font.pixelSize: chessGrid.tileSize
                    font.bold: true
                }
            }
        }
    }
}
