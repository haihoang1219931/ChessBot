import QtQuick 2.0

FocusScope {
    id: root
    focus: true
    width: 420
    height: 100
    property var chessModel: [
        {type:"k", symbol:"\u265A"},
        {type:"q", symbol:"\u265B"},
        {type:"r", symbol:"\u265C"},
        {type:"b", symbol:"\u265D"},
        {type:"n", symbol:"\u265E"},
        {type:"p", symbol:"\u265F"},
        {type:"K", symbol:"\u265A"},
        {type:"Q", symbol:"\u265B"},
        {type:"R", symbol:"\u265C"},
        {type:"B", symbol:"\u265D"},
        {type:"N", symbol:"\u265E"},
        {type:"P", symbol:"\u265F"},
        {type:".", symbol:"X"}
    ]

    property int selection: 0
    signal pieceSelected(var pieceType)
    signal cancelSelectPiece()
    Keys.onLeftPressed: {
        selection = (selection==0?(chessModel.length-1):selection-1)%chessModel.length;
    }
    Keys.onRightPressed: {
        selection = (selection+1)%chessModel.length;
    }
    Keys.onUpPressed: {
        selection = (selection<6?(chessModel.length-1):selection-6)%chessModel.length;
    }
    Keys.onDownPressed: {
        if(selection+6 < chessModel.length) {
            selection += 6;
        } else {
            if(selection != chessModel.length-1)
                selection = chessModel.length-1;
            else
                selection = 0;
        }
    }
    Keys.onReturnPressed: {
        pieceSelected(chessModel[selection].type);
    }
    Keys.onEscapePressed: {
        cancelSelectPiece();
    }

    Grid {
        id: chessGrid
        rows: 3
        columns: 6
        Repeater {
            model: root.chessModel
            Rectangle {
                width: 70
                height: 70
                color: "white"
                border.width: 1
                border.color: selection == index?"gray":"transparent"
                Text {
                    anchors.centerIn: parent
                    text: modelData.symbol
                    color: index <= 5?"gray" : "orange"
                    font.pixelSize: 70
                    font.bold: true
                }
            }
        }
    }
}
