import QtQuick 2.0
Grid {
    id: grid
    columns: 2
    rows: 8

    Repeater {
        id: repeater
        model: [0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15]
        delegate: Rectangle {
            width: grid.width / grid.columns
            height: grid.height / grid.rows
            color: "gray"
            border.color: "white"
            border.width: 1
        }
    }
}
