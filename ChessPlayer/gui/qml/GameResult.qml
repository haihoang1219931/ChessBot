import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

Rectangle {
    id: root
    width: 300
    height: 120
    color: "#88f8f9fa"
    property int gameResult: 0
    property var gameResultList: ["DRAW","WIN","LOST"]
    property int selection: 0
    signal gameNextStep(var nextStep);
    Keys.onLeftPressed: {
        selection = (selection+1)%2;
    }
    Keys.onRightPressed: {
        selection = (selection+1)%2;
    }
    Keys.onReturnPressed: {
        console.log("Gameresult select "+selection);
        gameNextStep(selection);
    }

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 10

        Text {
            text: gameResultList[gameResult]
            font.pixelSize: 24
            Layout.alignment: Qt.AlignHCenter
        }

        RowLayout {
            spacing: 50

            // Restart Button
            Rectangle {
                implicitWidth: 100
                implicitHeight: 50
                color: selection == 0? "green": "#1a5f7a"

                Text {
                    text: "Restart"
                    color: "white"
                    anchors.centerIn: parent
                    font.pixelSize: 20
                }
            }

            // Change Level Button
            Rectangle {
                implicitWidth: 100
                implicitHeight: 50
                color: selection == 1? "green": "#1a5f7a"

                Text {
                    text: "Change\nlevel"
                    color: "white"
                    anchors.centerIn: parent
                    font.pixelSize: 20
                }
            }
        }
    }
}
