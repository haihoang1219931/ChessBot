import QtQuick 2.12
import QtQuick.Shapes 1.12
import QtQuick.Controls 2.12

Rectangle {
    id: root
    width: 640
    height: 480
    color: "black"
    signal goback()
    signal startGame()
    signal gobackLevelSelection()
    Keys.onEscapePressed: root.goback()
    Keys.onReturnPressed: root.startGame()
    Keys.onSpacePressed: backend.randomMove()
    property int levelType: 1
    property int levelScore: 200
    property int side: 0
    property string player1Name: "Bot"
    property string player1Time: "02:51"
    property string player2Name: "Player"
    property string player2Time: "03:28"
    function openGameResult(result) {
        // 1. Set the source to your QML file
        if(myLoader.item === null)
        myLoader.setSource("GameResult.qml");
        myLoader.item.gameResult = result
    }
    function closeLoaderItem() {
        myLoader.source = "";   // This automatically destroys the loaded item
        root.forceActiveFocus(); // Restore focus to the main UI
    }
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
                anchors.top: parent.top
                height: parent.height
                anchors.leftMargin: 20
                spacing: 8

                Rectangle {
                    width: 18; height: 6
                    color: "white"
                    anchors.verticalCenter: parent.verticalCenter
                }

                Text {
                    text: "level "+levelType+" ("+levelScore+")"
                    color: "white"
                    font.pixelSize: 24
                    font.weight: Font.DemiBold
                    anchors.verticalCenter: parent.verticalCenter
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

                Column {
                    anchors.bottom: parent.bottom
                    anchors.right: parent.right
                    anchors.bottomMargin: 10
                    anchors.rightMargin: 10
                    spacing: -5
                    Text { text: player2Name; color: "white"; font.pixelSize: 52 }
                    Text { text: player2Time; color: "white"; font.pixelSize: 52; font.bold: true }
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
                    x: 5
                    spacing: -5
                    Text { text: player1Name; color: "white"; font.pixelSize: 52 }
                    Text { text: player1Time; color: "white"; font.pixelSize: 52; font.bold: true }
                }
            }
            ChessBoard {
                id: chessboard
                width: 300
                height: 300
                anchors.centerIn: parent
            }
        }
    }
    Loader {
        id: myLoader
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        focus: true // Necessary for children to receive focus

        onLoaded: {
            // 2. Force focus to the loaded item immediately after it's ready
            item.forceActiveFocus();
        }
    }

    Connections {
        target: myLoader.item // Connects to the loaded object
        ignoreUnknownSignals: true // Prevents errors before source is loaded

        onGameNextStep: {
            myLoader.source = ""; // Close it
            if(nextStep === 1) {
                console.log("gobackLevelSelection");
                gobackLevelSelection();
            } else {
                console.log("resetGame");
                backend.resetGame();
                root.forceActiveFocus();
            }
        }
    }
    Component.onCompleted: {
        root.levelType = backend.levelType
        root.levelScore = backend.levelScore
        root.side =  backend.side
    }
    Connections {
        target: backend
        onGameUpdated: {
            chessboard.updateChessBoard(newModel)
        }
        onGameEnded: {
            console.log("Game end: "+endState);
            openGameResult(endState);
        }
    }
}
