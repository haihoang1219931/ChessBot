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
            openHomeOption();
        }
    }
    Keys.onEscapePressed: {
        if(chessboard.activeUserInput) {
            console.log("chessboard.cancelUserSelection()");
            chessboard.cancelUserSelection();
        }
        else {
            console.log("masterBot.undoMove()");
            masterBot.undoMove();
        }
    }
    Keys.onReturnPressed: {
        root.startGame()
        if(!gameEnded) chessboard.updateUserSelection();
    }
    Keys.onSpacePressed: masterBot.processNextMove()
    Keys.onLeftPressed: chessboard.updateUserInput(-1)
    Keys.onRightPressed: chessboard.updateUserInput(1)
    Keys.onUpPressed: chessboard.updateUserInput(-8)
    Keys.onDownPressed: chessboard.updateUserInput(8)
    property string levelType: "Advanced"
    property int levelScore: 700
    property int side: 0
    property int gameTurn: 0
    property string player1Name: "Bot"
    property string player2Name: "Player"
    property int playTime: 0
    property int player1Time: 0
    property int player2Time: 0
    property int dirTime: 0
    property bool gameEnded: false;
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
        gameEnded = true;
        masterBot.stopGame(result === 2 ?"Player lost":"Player win");
    }

    function enablePromotionSelection(enable) {
        if(enable){
            // 1. Set the source to your QML file
            if(loaderDialogPromotion.item === null)
            loaderDialogPromotion.setSource("PromotionPieces.qml");
            loaderDialogPromotion.item.side = masterBot.playerColor() === 0 ?"white":"black";
        } else {
            loaderDialogPromotion.source = ""; // Close it
            root.forceActiveFocus();
        }
    }

    function openHomeOption() {
        // 1. Set the source to your QML file
        if(loaderDialogEndgame.item === null)
        loaderDialogEndgame.setSource("HomeOption.qml");
    }

    function openConfirmPlayOption() {
        // 1. Set the source to your QML file
        if(loaderDialogEndgame.item === null)
        loaderDialogEndgame.setSource("ConfirmPlay.qml");
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
                    text: levelType+" ("+levelScore+")"
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
        running: false
        repeat: true

        onTriggered: {
            if(gameTurn == side) {
                player2Time += dirTime
                if(player2Time == 0) {
                    openGameResult(2)
                    timer.stop();
                }
            } else {
                player1Time += dirTime
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
                masterBot.stopGame("Exit");
                gameEnded = true;
            } else {
                console.log("resetGame");
                masterBot.resetGame();
                root.resetGame();
                root.forceActiveFocus();
                gameEnded = true;
            }
        }

        onHomeNextStep: {
            loaderDialogEndgame.source = ""; // Close it
            if(nextStep === 0) {
                console.log("level selection");
                gobackLevelSelection();
                masterBot.stopGame("Exit");
                gameEnded = true;
            } else if(nextStep === 1){
                console.log("homing robot");
                masterBot.homingRobot();
                root.forceActiveFocus();
            }
        }

        onGoback: {
            loaderDialogEndgame.source = ""; // Close it
        }

        onConfirmNextStep: {
            loaderDialogEndgame.source = ""; // Close it
            console.log((nextStep === 0?"Confirm":"Reject")+" to play last FEN ");
            masterBot.acceptPlayFENFromHistory(nextStep === 0);
        }

        onGobackNormal: {
            loaderDialogEndgame.source = ""; // Close it
            console.log("Reject to play last FEN ");
            masterBot.acceptPlayFENFromHistory(false);
        }
    }

    Connections {
        target: loaderDialogPromotion.item // Connects to the loaded object
        ignoreUnknownSignals: true // Prevents errors before source is loaded

        onCancelSelectPromote: {
            enablePromotionSelection(false);
            masterBot.playInputCancelPromotion();
        }
        onPromoteSelected: {
            enablePromotionSelection(false);
            masterBot.playInputMove(chessboard.userInputIndexStart,
                                  chessboard.userInputIndexStop,
                                  promotePiece);
        }
    }

    Component.onCompleted: {
        root.levelType = chessController.engineLevel
        root.levelScore = chessController.engineElo
        root.side =  chessController.playerColor
        console.log("timeout: "+root.playTime)
        if(root.playTime != 0) {
            root.player1Time = root.playTime;
            root.player2Time = root.playTime;
            root.dirTime = -1;
        } else {
            root.dirTime = 1;
        }
        timer.start();
    }
    Connections {
        target: masterBot
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
        onFoundLastFEN: {
            console.log("Found last FEN");
            openConfirmPlayOption();
        }
        onBoardChanged: {
            chessboard.board = boardModel;
            chessboard.playerColor = chessController.playerColor
            chessboard.selectedSquare = chessController.selectedSquare
            chessboard.checkedKingSquare = chessController.checkedKingSquare
        }
    }
}
