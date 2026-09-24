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
    signal gobackLevelSelection()
    Keys.onPressed: {
        if (event.key === Qt.Key_Home) {
            console.log("Home key was pressed!");
            root.gobackLevelSelection();
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
//    Component.onCompleted: {
//        root.levelType = chessController.engineLevel
//        root.levelScore = chessController.engineElo
//        root.side =  chessController.playerColor
//    }
}
