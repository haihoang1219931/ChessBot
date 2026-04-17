import QtQuick 2.0
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.0

FocusScope {
    id: root
    width: 800
    height: 480
    
    property string calibrationFile: "trapezoid_data.json"
    property var chessboardCalib: []
    property var dropzoneRightCalib: []
    property var dropzoneLeftCalib: []
    property int cellSize: 40
    property int selectedRow: 0
    property int selectedCol: 0
    property int selectedZone: 0  // 0: chessboard, 1: dropzone right, 2: dropzone left
    
    signal exitPressed()
    focus: true
    
    Component.onCompleted: {
        loadCalibrationData();
    }

    onActiveFocusChanged: {
        if (activeFocus) {
            rectChessBoard.forceActiveFocus()
        }
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

    // Helper function for the corner markings
    function drawBrackets(ctx, w, h) {
        ctx.reset();
        ctx.strokeStyle = "#888";
        ctx.lineWidth = 6;
        var l = 25; // bracket length

        // Top Left
        ctx.beginPath(); ctx.moveTo(0, l); ctx.lineTo(0, 0); ctx.lineTo(l, 0); ctx.stroke();
        // Top Right
        ctx.beginPath(); ctx.moveTo(w - l, 0); ctx.lineTo(w, 0); ctx.lineTo(w, l); ctx.stroke();
        // Bottom Left
        ctx.beginPath(); ctx.moveTo(0, h - l); ctx.lineTo(0, h); ctx.lineTo(l, h); ctx.stroke();
        // Bottom Right
        ctx.beginPath(); ctx.moveTo(w - l, h); ctx.lineTo(w, h); ctx.lineTo(w, h - l); ctx.stroke();
    }
    Rectangle {
        id: rectChessBoard
        anchors.fill: parent
        radius: 20
        color: "#e8e8e8"
        border.color: "#aaa"; border.width: 1
        Keys.onEscapePressed: {
            exitPressed()
        }
        // Main horizontal container
        RowLayout {
            anchors.fill: parent
            anchors.margins: 40
            spacing: 30

            // --- LEFT COLUMN: ROUND BUTTONS AND 1x5/1x8 DROP ZONES ---
            ColumnLayout {
                spacing: 20

                // Left Technical Container
                Item {
                    width: 100; height: 400 // Fixed sizes for Qt 5.12 stability
                    GridLayout {
                        anchors.centerIn: parent
                        columns: 2; rows: 8
                        columnSpacing: 0; rowSpacing: 0
                        Repeater {
                            model: 16
                            Rectangle { width: 50; height: 50; color: "transparent"; border.color: "#bbb" }
                        }
                    }
                    Canvas {
                        anchors.fill: parent
                        onPaint: {
                            var ctx = getContext("2d");
                            drawBrackets(ctx, width, height);
                        }
                    }
                }
            }

            // --- CENTER COLUMN: THE 8x8 BOARD ---
            Rectangle {
                width: 400; height: 400
                Layout.alignment: Qt.AlignCenter
                border.color: "#333"; border.width: 2

                GridLayout {
                    anchors.fill: parent
                    anchors.margins: 1
                    columns: 8; rows: 8
                    columnSpacing: 0; rowSpacing: 0

                    Repeater {
                        model: 64
                        Button {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            // Basic Checkerboard logic
                            background: Rectangle {
                                color: (Math.floor(index / 8) + index % 8) % 2 === 0 ? "#ffffff" : "#222222"
                            }
                        }
                    }
                }
            }

            // --- RIGHT COLUMN: THE 2x8 DROP ZONE ---
            Item {
                width: 100
                height: 400

                GridLayout {
                    anchors.centerIn: parent
                    columns: 2; rows: 8
                    columnSpacing: 0; rowSpacing: 0
                    Repeater {
                        model: 16
                        Rectangle { width: 50; height: 50; color: "transparent"; border.color: "#bbb" }
                    }
                }
                Canvas {
                    anchors.fill: parent
                    onPaint: {
                        var ctx = getContext("2d");
                        drawBrackets(ctx, width, height);
                    }
                }
            }
        }
        Rectangle {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.topMargin: 40
            color: "#e8e8e8"
            width: 90
            height: 150
            ColumnLayout {
                spacing: 20
                Item {
                    width: 90
                    height: 30
                    Rectangle {
                        x: 25
                        width: 30
                        height: width
                        radius: width/2
                        border.color: "#888"
                        border.width: 2
                    }
                }

                Item {
                    width: 90
                    height: 50
                    Rectangle {
                        x: 15
                        width: 50
                        height: width
                        radius: width/2
                        border.color: "#888"
                        border.width: 2
                    }
                }

                Item {
                    width: 90
                    height: 50
                    RowLayout {
//                        spacing: 30
                        Rectangle {
                            Layout.leftMargin: 10
                            width: 20
                            height: width
                            radius: width/2
                            border.color: "#888"
                            border.width: 2
                        }
                        Rectangle {
                            Layout.leftMargin: 15
                            width: 20
                            height: width
                            radius: width/2
                            border.color: "#888"
                            border.width: 2
                        }
                    }
                }
            }
        }
    }
}
