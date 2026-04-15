import QtQuick 2.0
import QtQuick.Layouts 1.12

FocusScope {
    id: root
    width: 640
    height: 480
    signal exitPressed()
    
    property string calibrationFile: "trapezoid_data.json"
    property var chessboardCalib: []
    property var dropzoneRightCalib: []
    property var dropzoneLeftCalib: []
    property int cellSize: 40
    property int selectedRow: 0
    property int selectedCol: 0
    property int selectedZone: 0  // 0: chessboard, 1: dropzone right, 2: dropzone left
    
    focus: true
    
    Component.onCompleted: {
        loadCalibrationData();
    }
    
    function loadCalibrationData() {
        var data = fileio.read(calibrationFile);
        if (data !== "") {
            var jsonData = JSON.parse(data);
            
            // Load chessboard calibration (8x8)
            if (jsonData.chessboard) {
                chessboardCalib = [];
                for (var i = 0; i < 8; i++) {
                    chessboardCalib[i] = [];
                    for (var j = 0; j < 8; j++) {
                        chessboardCalib[i][j] = {x: 0, y: 0};
                    }
                }
                for (var k = 0; k < jsonData.chessboard.length; k++) {
                    var point = jsonData.chessboard[k];
                    chessboardCalib[point.row][point.col] = {x: point.x, y: point.y};
                }
            }
            
            // Load right dropzone calibration (8x2)
            if (jsonData.dropzone_right) {
                dropzoneRightCalib = [];
                for (var i = 0; i < 8; i++) {
                    dropzoneRightCalib[i] = [];
                    for (var j = 0; j < 2; j++) {
                        dropzoneRightCalib[i][j] = {x: 0, y: 0};
                    }
                }
                for (var k = 0; k < jsonData.dropzone_right.length; k++) {
                    var point = jsonData.dropzone_right[k];
                    dropzoneRightCalib[point.row][point.col] = {x: point.x, y: point.y};
                }
            }
            
            // Load left dropzone calibration (8x2)
            if (jsonData.dropzone_left) {
                dropzoneLeftCalib = [];
                for (var i = 0; i < 8; i++) {
                    dropzoneLeftCalib[i] = [];
                    for (var j = 0; j < 2; j++) {
                        dropzoneLeftCalib[i][j] = {x: 0, y: 0};
                    }
                }
                for (var k = 0; k < jsonData.dropzone_left.length; k++) {
                    var point = jsonData.dropzone_left[k];
                    dropzoneLeftCalib[point.row][point.col] = {x: point.x, y: point.y};
                }
            }
        }
    }
    
    function getCurrentPoint() {
        if (selectedZone === 0) {
            return chessboardCalib[selectedRow][selectedCol];
        } else if (selectedZone === 1) {
            return dropzoneRightCalib[selectedRow][selectedCol];
        } else {
            return dropzoneLeftCalib[selectedRow][selectedCol];
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "#050505"

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 15
            spacing: 10

            // Header
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 50
                color: "#1a1a1a"
                border.color: "#4488ff"
                border.width: 1
                radius: 5

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 20

                    Text {
                        text: "CALIBRATION POINTS VIEWER"
                        color: "white"
                        font.pixelSize: 18
                        font.family: "Orbitron"
                        font.bold: true
                    }

                    Item { Layout.fillWidth: true }

                    Text {
                        text: "Tab: Switch Zone | Arrow Keys: Navigate | Enter: Edit"
                        color: "#AAAAAA"
                        font.pixelSize: 10
                        font.family: "Courier"
                    }
                }
            }

            // Tab buttons for zones
            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                spacing: 10

                Repeater {
                    model: ["Chess Board (8x8)", "Drop Zone Right (8x2)", "Drop Zone Left (8x2)"]
                    
                    delegate: Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: selectedZone === index ? "#4488ff" : "#1a1a1a"
                        border.color: "#4488ff"
                        border.width: 1
                        radius: 5

                        Text {
                            anchors.centerIn: parent
                            text: modelData
                            color: selectedZone === index ? "#050505" : "#AAAAAA"
                            font.pixelSize: 12
                            font.family: "Orbitron"
                            font.bold: true
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                selectedZone = index;
                                selectedRow = 0;
                                selectedCol = 0;
                            }
                        }
                    }
                }
            }

            // Calibration grid
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#1a1a1a"
                border.color: "#4488ff"
                border.width: 1
                radius: 5

                Flickable {
                    id: flickable
                    anchors.fill: parent
                    anchors.margins: 10
                    contentWidth: gridContainer.width
                    contentHeight: gridContainer.height

                    Column {
                        id: gridContainer
                        spacing: 5

                        Repeater {
                            model: selectedZone === 0 ? 8 : 8
                            
                            delegate: Row {
                                spacing: 5
                                
                                Repeater {
                                    model: selectedZone === 0 ? 8 : 2
                                    
                                    delegate: Rectangle {
                                        width: cellSize
                                        height: cellSize
                                        color: (index === selectedCol && index2 === selectedRow && selectedZone === zoneIndex) ? "#4488ff" : "#2a2a2a"
                                        border.color: (index === selectedCol && index2 === selectedRow && selectedZone === zoneIndex) ? "#ffffff" : "#4488ff"
                                        border.width: 1
                                        radius: 3

                                        property int index2: index
                                        property int zoneIndex: selectedZone
                                        property var calibPoint: selectedZone === 0 ? chessboardCalib[index2][index] : 
                                                               (selectedZone === 1 ? dropzoneRightCalib[index2][index] : dropzoneLeftCalib[index2][index])

                                        Text {
                                            anchors.centerIn: parent
                                            text: calibPoint ? calibPoint.x.toString().substring(0, 4) : "?"
                                            color: (index === selectedCol && index2 === selectedRow && selectedZone === zoneIndex) ? "#050505" : "#AAAAAA"
                                            font.pixelSize: 8
                                            font.family: "Courier"
                                        }

                                        MouseArea {
                                            anchors.fill: parent
                                            onClicked: {
                                                selectedRow = index2;
                                                selectedCol = index;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Info panel
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 80
                color: "#1a1a1a"
                border.color: "#4488ff"
                border.width: 1
                radius: 5

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 5

                    Text {
                        text: "Selected Point: [" + selectedRow + ", " + selectedCol + "]"
                        color: "white"
                        font.pixelSize: 14
                        font.family: "Courier"
                        font.bold: true
                    }

                    Row {
                        spacing: 20
                        
                        Text {
                            text: "X: " + (getCurrentPoint() ? getCurrentPoint().x : "0")
                            color: "#AAAAAA"
                            font.pixelSize: 12
                            font.family: "Courier"
                        }
                        
                        Text {
                            text: "Y: " + (getCurrentPoint() ? getCurrentPoint().y : "0")
                            color: "#AAAAAA"
                            font.pixelSize: 12
                            font.family: "Courier"
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: "#0a0a0a"
                        radius: 3

                        Text {
                            anchors.centerIn: parent
                            text: "Press Esc to exit | Tab to switch zones"
                            color: "#666666"
                            font.pixelSize: 10
                            font.family: "Courier"
                        }
                    }
                }
            }
        }

        Keys.onTabPressed: {
            selectedZone = (selectedZone + 1) % 3;
            selectedRow = 0;
            selectedCol = 0;
        }

        Keys.onEscapePressed: {
            exitPressed();
        }

        Keys.onLeftPressed: {
            var maxCol = selectedZone === 0 ? 8 : 2;
            selectedCol = (selectedCol - 1 + maxCol) % maxCol;
        }

        Keys.onRightPressed: {
            var maxCol = selectedZone === 0 ? 8 : 2;
            selectedCol = (selectedCol + 1) % maxCol;
        }

        Keys.onUpPressed: {
            selectedRow = (selectedRow - 1 + 8) % 8;
        }

        Keys.onDownPressed: {
            selectedRow = (selectedRow + 1) % 8;
        }
    }
}
