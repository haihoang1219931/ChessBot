import QtQuick.Window 2.2
import QtQuick.Controls 2.0
import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQml 2.0

ApplicationWindow {
    id: wroot
    visible: true
    width: 800
    height: 480
    title: qsTr("ChessPlayer")
    color: "#050505"

    StackView {
        id: stack
        anchors.fill: parent
        // 1. MUST HAVE FOCUS TRUE
        focus: true

        // 2. Use initialItem instead of just nesting the item
//        initialItem: levelSelection

        // Force focus to the current item whenever the stack changes
        onCurrentItemChanged: if (currentItem) currentItem.forceActiveFocus()

        Component.onCompleted: {
            stack.push(downloadProgressLoader)
        }
    }

    Component {
        id: downloadProgressLoader
        DownloadProgress {
            onOverlayHidden: {
                stack.pop()
                stack.push(menuSelection)
            }
            Component.onCompleted: {
                backend.initRobotCommunication();
            }
        }
    }

    Component {
        id: menuSelection
        StartupMenu {
            onEnterItem: {
                if(item === 0) {
                    stack.pop()
                    stack.push(levelSelection)
                } else if(item === 1) {
                    stack.pop()
                    stack.push(settingsMenu)
                }
            }
        }
    }

    Component {
        id: settingsMenu
        SettingsMenu {
            onExitPressed: {
                stack.pop()
                stack.push(menuSelection)
            }
            onSelectCalibration: {
                stack.pop()
                if (calibType === "camera") {
                    stack.push(calibPanel)
                } else if (calibType === "chessboard") {
                    stack.push(calibrationPointsPanel)
                }
            }
        }
    }

    Component {
        id: calibPanel
        SettingCalibChessBoard {
            onExitPressed: {
                stack.pop()
                stack.push(settingsMenu)
            }
        }
    }

    Component {
        id: calibrationPointsPanel
        SettingCalibrationPoints {
            onExitPressed: {
                stack.pop()
                stack.push(settingsMenu)
            }
        }
    }

    Component {
        id: emoji
        Emoji {
            onEnterPressed:{
                stack.pop()
                stack.push(levelSelection)
            }
        }
    }

    Component {
        id: levelSelection
        LevelSelection {
            onItemSelected: {
                stack.pop()
                stack.push(sideSelection)
                backend.setLevel(score)
            }
            onExitPressed: {
                stack.pop()
                stack.push(menuSelection)
            }
        }
    }

    Component {
        id: sideSelection
        SideSelection{
            onGoback: {
                stack.pop()
                stack.push(levelSelection)
            }
            onSideConfirmed: {
                stack.pop();
                stack.push(timer, {
                               "side":side==="White"?0:1,
                               "gameTurn":side==="White"?0:1,
                               "playTime": 600,
                               "player1Time": 600,
                               "player2Time": 600,
                           });
                backend.setSide(side==="White"?0:1);
                backend.resetGame()
            }
        }
    }
    Component {
        id: timer
        CountDownTimer{
            onGoback: {
                stack.pop()
                stack.push(sideSelection)
            }
            onGobackLevelSelection: {
                stack.pop()
                stack.push(levelSelection)
            }
        }
    }
}
