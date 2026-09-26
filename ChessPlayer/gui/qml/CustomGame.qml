import QtQuick 2.12
import QtQuick.Shapes 1.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

Rectangle {
    id: root
    width: 800
    height: 480
    color: "black"
    property string levelType: "Advanced"
    property int levelScore: 700
    property int side: 0
    property int gameTurn: 0
    property bool gameEnded: false
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
        if(!gameEnded) chessboard.updateUserSelection();
    }
    Keys.onSpacePressed: masterBot.processNextMove()
    Keys.onLeftPressed: chessboard.updateUserInput(-1)
    Keys.onRightPressed: chessboard.updateUserInput(1)
    Keys.onUpPressed: chessboard.updateUserInput(-8)
    Keys.onDownPressed: chessboard.updateUserInput(8)
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
        if(loaderDialogEndgame.item === null) {
            root.focus = false;
            loaderDialogEndgame.setSource("HomeOptionWithDetect.qml");
        }
    }

    function openConfirmPlayOption() {
        // 1. Set the source to your QML file
        if(loaderDialogEndgame.item === null) {
            root.focus = false;
            loaderDialogEndgame.setSource("ConfirmPlay.qml");
        }
    }
    ColumnLayout {
        spacing: 0
        Rectangle {
            id: topBar
            Layout.preferredWidth: root.width
            Layout.preferredHeight:60
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
        RowLayout {
            Layout.preferredWidth: root.width
            Layout.preferredHeight: root.height - topBar.height
            Item {
                width: root.width * 2/3
                height: root.height - topBar.height
                ChessBoard {
                    id: chessboard
                    anchors.centerIn: parent
                    width: 400
                    height: 400
                }
            }
            ListView {
                width: root.width * 1/3
                height: root.height - topBar.height
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
                root.forceActiveFocus();
                gameEnded = true;
            }
        }
        onHomeNextStep: {
            loaderDialogEndgame.source = ""; // Close it
            if(nextStep === 2) {
                console.log("level selection");
                gobackLevelSelection();
                masterBot.stopGame("Exit");
                gameEnded = true;
            } else if(nextStep === 1){
                console.log("homing robot");
                masterBot.homingRobot();
                root.forceActiveFocus();
            } else if(nextStep === 0){
                console.log("Reset game");
                masterBot.resetGame();
            }
        }

        onGoback: {
            loaderDialogEndgame.source = ""; // Close it
            root.forceActiveFocus();
        }
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
        onBoardChanged: {
            chessboard.board = boardModel;
            chessboard.playerColor = chessController.playerColor
            chessboard.selectedSquare = chessController.selectedSquare
            chessboard.checkedKingSquare = chessController.checkedKingSquare
        }
    }
    Component.onCompleted: {
        root.levelType = chessController.engineLevel
        root.levelScore = chessController.engineElo
        root.side =  chessController.playerColor
    }
}
