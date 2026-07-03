import QtQuick 2.0

Item {
    id: root
    width: 300
    height: 100
    property string side: "white"
    property int selection: 0
    signal promoteSelected(var promotePiece)
    signal cancelSelectPromote()
    Keys.onLeftPressed: {
        selection = (selection==0?3:selection-1)%4;
    }
    Keys.onRightPressed: {
        selection = (selection+1)%4;
    }
    Keys.onReturnPressed: {
        promoteSelected(selection);
    }
    Keys.onEscapePressed: {
        cancelSelectPromote();
    }

    Row {
        id: chessGrid
        Repeater {
            model: ["\u265B","\u265C","\u265E","\u265D"]

            Rectangle {
                width: 100
                height: 100
                color: "white"
                border.width: 1
                border.color: selection == index?"gray":"transparent"
                Text {
                    anchors.centerIn: parent
                    text: modelData
                    color: side == "white"?"gray" : "orange"
                    font.pixelSize: 100
                    font.bold: true
                }
            }
        }
    }
}
