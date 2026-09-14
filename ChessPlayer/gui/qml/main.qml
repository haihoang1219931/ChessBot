import QtQuick.Window 2.2
import QtQuick.Controls 2.0
import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQml 2.0

ApplicationWindow {
    id: wroot
    visible: true
//    visibility: Window.FullScreen
    title: qsTr("ChessPlayer")
    color: "#050505"
    width: 800
    height: 480
    StackView {
        id: stack
        width: 800
        height: 480
        anchors.centerIn: parent
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
                masterBot.initRobotCommunication();
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
                } else if (calibType === "command") {
                    stack.push(testCommandPanel)
                } else if (calibType === "vision") {
                    stack.push(testBotVision)
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
        id: testCommandPanel
        SettingTestCommand {
            onExitPressed: {
                stack.pop()
                stack.push(settingsMenu)
            }
        }
    }

    Component {
        id: testBotVision
        BotVision {
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
                stack.push(timeSelection)
                masterBot.setEngineElo(rank,score)
            }
            onExitPressed: {
                stack.pop()
                stack.push(menuSelection)
            }
        }
    }

    Component {
        id: timeSelection
        GameTypeSelection{
            onExitPressed: {
                stack.pop()
                stack.push(levelSelection)
            }
            onItemSelected: {
                if(expireTime === "30 mins")
                    masterBot.setTimeLimit(30*60);
                else if(expireTime === "10 mins")
                    masterBot.setTimeLimit(10*60);
                else
                    masterBot.setTimeLimit(0);
                stack.pop();
                stack.push(sideSelection);
            }
        }
    }

    Component {
        id: sideSelection
        SideSelection{
            onGoback: {
                stack.pop()
                stack.push(timeSelection)
            }
            onSideConfirmed: {
                stack.pop();
                stack.push(timer, {
                               "side":side==="White"?0:1,
                               "gameTurn":side==="White"?0:1,
                               "playTime": masterBot.timerLimit(),
                           });
                masterBot.setPlayerColor(side==="White"?0:1);
                masterBot.resetGame()
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
    Component.onCompleted: {
        masterBot.startService();
    }
}
