import QtQuick 2.0
import "qrc:/qml/ChessSupport.js" as ChessSupport
Item {
    id: root
    property real chessRotation: 0
    function updateChessBoard(newModel) {
        repeaterChessPiece.model = newModel;
//        repeaterChessPiece.model = ChessSupport.createChessPiecesModel();
//        console.log("chesssupport: "+ChessSupport.createChessPiecesModel());
    }

    Grid {
        id: grid
        columns: 8
        rows: 8
        rotation: chessRotation
        Repeater {
            id: repeater
            delegate: Rectangle {
                width: root.width / grid.columns
                height: root.height / grid.rows
                color: (modelData.row+modelData.col)%2 === 0? "#90652C" : "#DEB887"
            }
        }
    }
    Grid {
        id: gridChessPiece
        columns: 8
        rows: 8
        rotation: chessRotation
        Repeater {
            id: repeaterChessPiece
            delegate: Text {
                width: root.width / grid.columns
                height: root.height / grid.rows
                rotation: -chessRotation
                text: ChessSupport.charCode(parseInt(modelData))
//                text: modelData
                font.pointSize : root.width / grid.columns * 0.6
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
    }
    Component.onCompleted: {
        repeater.model = ChessSupport.createChessBoardModel();
    }
}
