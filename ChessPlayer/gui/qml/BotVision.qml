import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12

FocusScope {
    id: root
    width: 800
    height: 480
    property var botBoard: []
    property var playerBoard: []
    property int viewId: 0
    property var viewList: ["Bot view","Player view"]
    signal exitPressed()
    focus: true

    // Fallback escape handler for the root scope
    Keys.onEscapePressed: root.exitPressed();
    Keys.onSpacePressed: root.analyzeChessboard();
    Keys.onReturnPressed: root.analyzeChessboard();
    Keys.onLeftPressed: {
        viewId = (viewId-1) >= 0?(viewId-1)%2:1;
    }
    Keys.onRightPressed: {
        viewId = (viewId+1)%2;
    }
    function analyzeChessboard() {
        console.log("analyzeChessboard")
        masterBot.classifyImage();
    }

    function pieceText(code) {
        switch (code) {
        case "K": return "\u265A"
        case "Q": return "\u265B"
        case "R": return "\u265C"
        case "B": return "\u265D"
        case "N": return "\u265E"
        case "P": return "\u265F"
        case "k": return "\u265A"
        case "q": return "\u265B"
        case "r": return "\u265C"
        case "b": return "\u265D"
        case "n": return "\u265E"
        case "p": return "\u265F"
        default: return "."
        }
    }

    function isFirstLetterUppercase(str) {
      // Ensure the string isn't empty before testing
      if (!str) return false;

      return /^[A-Z]/.test(str);
    }

    function pieceColor(code) {
        if (code.charAt(0) === '.') return "transparent"
        return isFirstLetterUppercase(code) ? "orange":"gray"
    }
    ColumnLayout {
        anchors.fill: parent
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 50
            Rectangle {
                implicitWidth: 50
                implicitHeight: 50
                color: "green"
                Text {
                    text: "<"
                    color: "white"
                    anchors.centerIn: parent
                    font.pixelSize: 20
                }
            }

            Rectangle {
                implicitWidth: 200
                implicitHeight: 50
                color: "green"

                Text {
                    text: root.viewList[root.viewId]
                    color: "white"
                    anchors.centerIn: parent
                    font.pixelSize: 20
                }
            }

            Rectangle {
                implicitWidth: 50
                implicitHeight: 50
                color: "green"
                Text {
                    text: ">"
                    color: "white"
                    anchors.centerIn: parent
                    font.pixelSize: 20
                }
            }
        }

        Item {
            id: chessResult
            Layout.alignment: Qt.AlignHCenter
            implicitWidth: 50 * 14
            implicitHeight: 50 * 8
//            Image {
//                id: imgWarpedImage
//                anchors.fill: parent
//            }
            Grid {
                id: chessGrid
                anchors.centerIn: parent
                columns: 14; rows: 8
                property real cellWidth: parent.width / 14
                property real cellHeight: parent.height / 8
                Repeater {
                    model: 112
                    Rectangle {
                        width: chessGrid.cellWidth
                        height: chessGrid.cellHeight
                        color: {
//                            if(viewId === 0) return "transparent"
                            var isChessBoard = index % 14 >= 3 && index % 14 <= 10
                            var rank = Math.floor(index / 14)
                            var file = index % 14-2
                            var light = ((rank + file) % 2) === 1
                            return isChessBoard ? (light ? "white" : "black"):"white"
                        }
                        border.color: {
//                            if(viewId === 0) return "gray"
                            var isRightDropZone = viewId === 0 ?
                                        index%14>=12 : index%14>=12 && index/14<5;
                            var isLeftDropZone = viewId === 0 ?
                                        index%14<=1 && index/14.0>=3:index%14<=1;
                            return isRightDropZone || isLeftDropZone ? "gray":"transparent"
                        }
                        border.width: 1

                        Text {
                            anchors.centerIn: parent

                            // Choose the correct 1D array depending on viewId (0 for Bot, 1 for Player)
                            property var activeBoard: root.viewId === 0 ? root.botBoard : root.playerBoard

                            text: pieceText(activeBoard && activeBoard[index] ? activeBoard[index] : "")
                            color: pieceColor(activeBoard && activeBoard[index] ? activeBoard[index] : ".")
                            font.pixelSize: chessGrid.cellHeight
                            font.bold: true
                        }

                    }
                }
            }
            Rectangle {
                anchors.centerIn: parent
                implicitWidth: 50 * 8
                implicitHeight: 50 * 8
                color: "transparent"
                border.color: "gray"
                border.width: 1
            }
        }
    }
    Connections {
        target: masterBot
//        onPreprocessDone: {
//            imgWarpedImage.source = "file:///" +
//                    applicationDirPath +"/"+
//                    imagePath;
//        }

        onClassificationDone: {
            console.log("BotVision update classification result");
            root.botBoard = boardModel;
            root.playerBoard = boardModelReverted;
        }
    }
}
