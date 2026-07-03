import QtQuick 2.12
import QtQuick.Shapes 1.12
import QtQuick.Controls 2.12

Rectangle {
    id: root
    width: 800
    height: 480
    color: "black"
    signal goback()
    signal startGame()
    signal gobackLevelSelection()
    Keys.onPressed: {
        if (event.key === Qt.Key_Home) {
            console.log("Home key was pressed!");
            root.goback();
        }
    }
    Keys.onEscapePressed: {
        if(chessboard.activeUserInput)
            chessboard.cancelUserSelection();
        else
            backend.undoMove();
    }
    Keys.onReturnPressed: {
        root.startGame()
        chessboard.updateUserSelection();
    }
    Keys.onSpacePressed: backend.processNextMove()
    Keys.onLeftPressed: chessboard.updateUserInput(-1)
    Keys.onRightPressed: chessboard.updateUserInput(1)
    Keys.onUpPressed: chessboard.updateUserInput(-8)
    Keys.onDownPressed: chessboard.updateUserInput(8)
    property int levelType: 1
    property int levelScore: 200
    property int side: 0
    property int gameTurn: 0
    property string player1Name: "Bot"
    property string player2Name: "Player"
    property int playTime: 600
    property int player1Time: 600
    property int player2Time: 600
    function resetGame(){
        player1Time = playTime;
        player2Time = playTime;
        gameTurn = side;
        timer.start();
        console.log("Reset game gameTurn="+gameTurn);
    }

    function formatSeconds(totalSeconds) {
        const minutes = Math.floor(totalSeconds / 60);
        const seconds = totalSeconds % 60;

        const paddedMinutes = String(minutes).padStart(2, '0');
        const paddedSeconds = String(seconds).padStart(2, '0');

        return paddedMinutes+":"+paddedSeconds;
    }

    function openGameResult(result) {
        // 1. Set the source to your QML file
        if(loaderDialogEndgame.item === null)
        loaderDialogEndgame.setSource("GameResult.qml");
        loaderDialogEndgame.item.gameResult = result
    }

    function enablePromotionSelection(enable) {
        if(enable){
            // 1. Set the source to your QML file
            if(loaderDialogPromotion.item === null)
            loaderDialogPromotion.setSource("PromotionPieces.qml");
            loaderDialogPromotion.item.side = backend.side === 0 ?"white":"black";
        } else {
            loaderDialogPromotion.source = ""; // Close it
            root.forceActiveFocus();
        }
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
                id: rectRight
                anchors.fill: parent
                color: "#001e3e"
                property real textSizeFactor: root.gameTurn != root.side ?52:72
                Behavior on textSizeFactor {
                    NumberAnimation { duration: 300; easing.type: Easing.InOutQuad }
                }
                Column {
                    anchors.bottom: parent.bottom
                    anchors.right: parent.right
                    anchors.bottomMargin: 10
                    anchors.rightMargin: 10
                    spacing: -5
                    Text { text: player2Name; color: "white"; font.pixelSize: rectRight.textSizeFactor}
                    Text { text: formatSeconds(player2Time); color: "white"; font.pixelSize: rectRight.textSizeFactor; font.bold: true }
                }
            }

            // Red/Orange Shape with Slash (Left)
            Shape {
                id: dynamicShape
                anchors.fill: parent
                smooth: true
                property real widthFactor: root.gameTurn == root.side ? 0.42 : 0.82
                property real textSizeFactor: root.gameTurn != root.side ?72:52
                Behavior on widthFactor {
                    NumberAnimation { duration: 300; easing.type: Easing.InOutQuad }
                }
                Behavior on textSizeFactor {
                    NumberAnimation { duration: 300; easing.type: Easing.InOutQuad }
                }
                ShapePath {
                    strokeWidth: 0
                    fillGradient: LinearGradient {
                        x1: 0; y1: 0; x2: root.width; y2: root.height
                        GradientStop { position: 0.0; color: "#9a1a00" }
                        GradientStop { position: 0.6; color: "#ff4d00" }
                    }

                    startX: 0; startY: 0
                    PathLine { x: root.width * dynamicShape.widthFactor; y: 0 }
                    PathLine { x: root.width * (dynamicShape.widthFactor-0.2); y: root.height }
                    PathLine { x: 0; y: root.height }
                    PathLine { x: 0; y: 0 }
                }

                Column {
                    x: 5
                    spacing: -5
                    Text { text: player1Name; color: "white"; font.pixelSize: dynamicShape.textSizeFactor }
                    Text { text: formatSeconds(player1Time); color: "white"; font.pixelSize: dynamicShape.textSizeFactor; font.bold: true }
                }
            }
            ChessBoard {
                id: chessboard
                width: 400
                height: 400
                anchors.centerIn: parent
                controller: backend ? backend.chessController : null
            }
        }
    }

    Loader {
        id: loaderDialogEndgame
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        focus: true // Necessary for children to receive focus

        onLoaded: {
            // 2. Force focus to the loaded item immediately after it's ready
            item.forceActiveFocus();
        }
    }

    Loader {
        id: loaderDialogPromotion
        anchors.centerIn: parent
        focus: true // Necessary for children to receive focus

        onLoaded: {
            // 2. Force focus to the loaded item immediately after it's ready
            item.forceActiveFocus();
        }
    }

    Timer {
        id: timer
        interval: 1000
        running: true
        repeat: true

        onTriggered: {
            if(gameTurn == side) {
                player2Time --
                if(player2Time == 0) {
                    openGameResult(2)
                    timer.stop();
                }
            } else {
                player1Time --
                if(player1Time == 0) {
                    openGameResult(1)
                    timer.stop();
                }
            }
        }
    }

    Connections {
        target: loaderDialogEndgame.item // Connects to the loaded object
        ignoreUnknownSignals: true // Prevents errors before source is loaded

        onGameNextStep: {
            loaderDialogEndgame.source = ""; // Close it
            if(nextStep === 1) {
                console.log("gobackLevelSelection");
                gobackLevelSelection();
            } else {
                console.log("resetGame");
                backend.resetGame();
                root.resetGame();
                root.forceActiveFocus();
            }
        }
    }

    Connections {
        target: loaderDialogPromotion.item // Connects to the loaded object
        ignoreUnknownSignals: true // Prevents errors before source is loaded

        onCancelSelectPromote: {
            enablePromotionSelection(false);
            backend.playInputCancelPromotion();
        }
        onPromoteSelected: {
            enablePromotionSelection(false);
            backend.playInputMove(chessboard.userInputIndexStart,
                                  chessboard.userInputIndexStop,
                                  promotePiece);
        }
    }

    Component.onCompleted: {
        root.levelType = backend.levelType
        root.levelScore = backend.levelScore
        root.side =  backend.side
    }
    Connections {
        target: backend
        onGameEnded: {
            console.log("Game end: "+endState);
            openGameResult(endState);
        }
        onDetectFailed: {
            chessboard.enableUserInput(true);
        }
        onShowPromotionPieces: {
            enablePromotionSelection(true);
        }
        onPlayTurnChanged:{
            root.gameTurn = root.side == 0 ? nextTurn:1-nextTurn;
        }
    }
}
