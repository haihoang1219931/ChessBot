import QtQuick 2.0
import "qrc:/qml/ChessSupport.js" as ChessSupport
Item {
    id: root
    property real chessRotation: 0
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
                Rectangle {
                    anchors.fill: parent
                    color: "transparent"
                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        onEntered: {
                            parent.color = "yellow"
                        }
                        onExited: {
                            parent.color = "transparent"
                        }
                        onClicked: {
                            var itemClicked = repeaterChessPiece.model[index];
                            var rowClicked = modelData.row;
                            var colClicked = modelData.col;
                            console.log("itemClicked:"+itemClicked+"("+rowClicked+","+colClicked+") Clicked: " +
                                        repeater.model[rowClicked*8+colClicked].clicked + " Sugguest "+repeater.model[rowClicked*8+colClicked].suggest);
                            if(repeater.model[rowClicked*8+colClicked].sugguest) {
                                var chessPieceModel = ChessSupport.updateChessPiece(repeaterChessPiece.model,
                                                                                    repeater.model,rowClicked,colClicked);
                            }

                            var chessBoardModel;
                            chessBoardModel = ChessSupport.updateSugguestMoves(repeater.model,rowClicked,colClicked);
                            repeater.model = chessBoardModel;
                        }
                    }
                }
//                Rectangle {
//                    anchors.verticalCenter: parent.verticalCenter
//                    anchors.horizontalCenter: parent.horizontalCenter
//                    color: "transparent"
//                    border.color: modelData.suggest?"gray":"transparent"
//                    border.width: 10
//                    width: ChessSupport.isOppositeSide(repeaterChessPiece.model[index],chessClicked)?parent.width:20
//                    height: ChessSupport.isOppositeSide(repeaterChessPiece.model[index],chessClicked)?parent.height:20
//                    radius: width/2
//                }
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
                text: ChessSupport.charCode(modelData)
                font.pointSize : root.width / grid.columns * 0.6
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
    }
    Component.onCompleted: {
        repeater.model = ChessSupport.createChessBoardModel();
        repeaterChessPiece.model = ChessSupport.createChessPiecesModel();
    }
}
