import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

Rectangle {
    id: root
    width: 450
    height: 120
    color: "#88f8f9fa"
    property int selection: 0
    signal confirmNextStep(var nextStep)
    signal gobackNormal()
    Keys.onLeftPressed: {
        selection = (selection-1) >= 0?(selection-1)%3:2;
    }

    Keys.onUpPressed: {
        selection = (selection-1) >= 0?(selection-1)%3:2;
    }

    Keys.onRightPressed: {
        selection = (selection+1)%3;
    }

    Keys.onDownPressed: {
        selection = (selection+1)%3;
    }

    Keys.onReturnPressed: {
        console.log("Confirm analyze next step "+selection);
        confirmNextStep(selection);
    }
    Keys.onSpacePressed: {
        console.log("Confirm analyze next step "+selection);
        confirmNextStep(selection);
    }

    Keys.onEscapePressed: {
        gobackNormal()
    }

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 10

        Text {
            text: "Analyzation done!"
            color: "white"
            font.bold: true
            font.pixelSize: 24
            Layout.alignment: Qt.AlignHCenter
        }

        RowLayout {
            spacing: 50

            // Back Button
            Rectangle {
                implicitWidth: 100
                implicitHeight: 50
                color: selection == 0? "green": "#1a5f7a"

                Text {
                    text: "Start"
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
                    text: "Set\npieces"
                    color: "white"
                    anchors.centerIn: parent
                    font.pixelSize: 20
                }
            }
            // Change Level Button
            Rectangle {
                implicitWidth: 100
                implicitHeight: 50
                color: selection == 2? "green": "#1a5f7a"

                Text {
                    text: "Analyze\nagain"
                    color: "white"
                    anchors.centerIn: parent
                    font.pixelSize: 20
                }
            }
        }
    }
}
