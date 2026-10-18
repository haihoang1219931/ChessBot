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
    property string editMode: "" // 2 modes ["setup", "play"]
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
        if(root.editMode == "play") {
            if(!gameEnded) chessboard.updateUserSelection();
        } else if(root.editMode == "setup") {
            openSetPiecesOption();
        }
    }
    Keys.onSpacePressed: {
        if(root.editMode == "play") {
            masterBot.processNextMove()
        } else if(root.editMode == "setup") {
            openConfirmPlayOption();
        }
    }
    Keys.onLeftPressed: chessboard.updateUserInput(-1)
    Keys.onRightPressed: chessboard.updateUserInput(1)
    Keys.onUpPressed: chessboard.updateUserInput(-8)
    Keys.onDownPressed: chessboard.updateUserInput(8)
    function openGameResult(result) {
        // 1. Set the source to your QML file
        if(loaderDialog.item === null)
        loaderDialog.setSource("GameResult.qml");
        loaderDialog.item.gameResult = result
        gameEnded = true;
        masterBot.stopGame(result === 2 ?"Player lost":"Player win");
    }

    function enablePromotionSelection(enable) {
        if(enable){
            // 1. Set the source to your QML file
            if(loaderDialog.item === null)
            loaderDialog.setSource("PromotionPieces.qml");
            loaderDialog.item.side = masterBot.playerColor() === 0 ?"white":"black";
        } else {
            loaderDialog.source = ""; // Close it
            root.forceActiveFocus();
        }
    }

    function openHomeOption() {
        // 1. Set the source to your QML file
        if(loaderDialog.item === null) {
            root.focus = false;
            loaderDialog.setSource("HomeOptionWithDetect.qml");
        }
    }

    function openConfirmPlayOption() {
        // 1. Set the source to your QML file
        if(loaderDialog.item === null) {
            root.focus = false;
            loaderDialog.setSource("ConfirmAnalyzeNextStep.qml");
        }
    }

    function openSetPiecesOption() {
        // 1. Set the source to your QML file
        if(loaderDialog.item === null) {
            root.focus = false;
            loaderDialog.setSource("SetupPieces.qml");
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
        id: loaderDialog
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        focus: true // Necessary for children to receive focus

        onLoaded: {
            // 2. Force focus to the loaded item immediately after it's ready
            item.forceActiveFocus();
        }
    }

    Connections {
        target: loaderDialog.item // Connects to the loaded object
        ignoreUnknownSignals: true // Prevents errors before source is loaded

        onGameNextStep: {
            loaderDialog.source = ""; // Close it
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
            loaderDialog.source = ""; // Close it
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
        onConfirmNextStep: {
            loaderDialog.source = ""; // Close it
            if(nextStep === 0) {
                root.editMode = "play"
                console.log("startCustomGame");
                masterBot.startCustomGame(chessboard.board);
                chessboard.enableUserInput(false)
            } else if(nextStep === 1) {
                root.editMode = "setup"
                chessboard.enableUserInput(true);
            } else if(nextStep === 2) {
                root.editMode = "setup"
                masterBot.resetGame();
                chessboard.enableUserInput(false)
            }
        }

        onCancelSelectPiece: {
            loaderDialog.source = "";
        }
        onPieceSelected: {
            loaderDialog.source = "";
            chessboard.changeSelectedCell(pieceType);
        }
        onPromoteSelected: {
            loaderDialog.source = "";
            masterBot.playInputMove(chessboard.userInputIndexStart,
                                  chessboard.userInputIndexStop,
                                  promotePiece);
            chessboard.enableUserInput(false);
        }

        onGoback: {
            loaderDialog.source = ""; // Close it
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
        onClassificationDone: {
            chessboard.board = boardModel;
            openConfirmPlayOption();
        }
    }
    Component.onCompleted: {
        root.levelType = chessController.engineLevel
        root.levelScore = chessController.engineElo
        root.side =  chessController.playerColor
    }
}
