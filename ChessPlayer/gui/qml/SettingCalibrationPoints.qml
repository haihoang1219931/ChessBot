import QtQuick 2.0
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.0
import QtGraphicalEffects 1.0
import QtQml 2.0

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
    property int adjustValue: 50
    // Excluded cells configuration - developers can modify these arrays
    // Each object contains {actualRow, actualCol} coordinates
    // If array is empty, all cells are selectable
    property var excludedLeftDropzoneCells: [
        
    ]
    property var excludedRightDropzoneCells: [
        {actualRow: 5, actualCol: 1},
        {actualRow: 6, actualCol: 1},
        {actualRow: 7, actualCol: 1}
    ]
    
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

    function initializeCalibrationArrays() {
        chessboardCalib = [];
        for (var i = 0; i < 8; i++) {
            chessboardCalib[i] = [];
            for (var j = 0; j < 8; j++) {
                chessboardCalib[i][j] = {x: 0, y: 0};
            }
        }

        dropzoneRightCalib = [];
        dropzoneLeftCalib = [];
        for (var i = 0; i < 8; i++) {
            dropzoneRightCalib[i] = [];
            dropzoneLeftCalib[i] = [];
            for (var j = 0; j < 2; j++) {
                dropzoneRightCalib[i][j] = {x: 0, y: 0};
                dropzoneLeftCalib[i][j] = {x: 0, y: 0};
            }
        }
    }

    function loadCalibrationData() {
        initializeCalibrationArrays();
        var data = masterBot.getCalibrationJson() || "";
        if (data !== "") {
            var jsonData = JSON.parse(data);
            
            // Load chessboard calibration (8x8 matrix)
            if (jsonData.chessboard) {
                chessboardCalib = [];
                for (var i = 0; i < 8; i++) {
                    chessboardCalib[i] = [];
                    for (var j = 0; j < 8; j++) {
                        chessboardCalib[i][j] = {x: 0, y: 0};
                    }
                }
                if (Array.isArray(jsonData.chessboard[0])) {
                    for (var r = 0; r < Math.min(8, jsonData.chessboard.length); r++) {
                        for (var c = 0; c < Math.min(8, jsonData.chessboard[r].length); c++) {
                            var point = jsonData.chessboard[r][c];
                            chessboardCalib[r][c] = {x: point.x || 0, y: point.y || 0};
                        }
                    }
                } else {
                    for (var k = 0; k < jsonData.chessboard.length; k++) {
                        var point = jsonData.chessboard[k];
                        chessboardCalib[point.row][point.col] = {x: point.x, y: point.y};
                    }
                }
            }
            
            // Load right dropzone calibration (8x2 matrix)
            if (jsonData.dropzone_right) {
                dropzoneRightCalib = [];
                for (var i = 0; i < 8; i++) {
                    dropzoneRightCalib[i] = [];
                    for (var j = 0; j < 2; j++) {
                        dropzoneRightCalib[i][j] = {x: 0, y: 0};
                    }
                }
                if (Array.isArray(jsonData.dropzone_right[0])) {
                    for (var r = 0; r < Math.min(8, jsonData.dropzone_right.length); r++) {
                        for (var c = 0; c < Math.min(2, jsonData.dropzone_right[r].length); c++) {
                            var point = jsonData.dropzone_right[r][c];
                            dropzoneRightCalib[r][c] = {x: point.x || 0, y: point.y || 0};
                        }
                    }
                } else {
                    for (var k = 0; k < jsonData.dropzone_right.length; k++) {
                        var point = jsonData.dropzone_right[k];
                        dropzoneRightCalib[point.row][point.col] = {x: point.x, y: point.y};
                    }
                }
            }
            
            // Load left dropzone calibration (8x2 matrix)
            if (jsonData.dropzone_left) {
                dropzoneLeftCalib = [];
                for (var i = 0; i < 8; i++) {
                    dropzoneLeftCalib[i] = [];
                    for (var j = 0; j < 2; j++) {
                        dropzoneLeftCalib[i][j] = {x: 0, y: 0};
                    }
                }
                if (Array.isArray(jsonData.dropzone_left[0])) {
                    for (var r = 0; r < Math.min(8, jsonData.dropzone_left.length); r++) {
                        for (var c = 0; c < Math.min(2, jsonData.dropzone_left[r].length); c++) {
                            var point = jsonData.dropzone_left[r][c];
                            dropzoneLeftCalib[r][c] = {x: point.x || 0, y: point.y || 0};
                        }
                    }
                } else {
                    for (var k = 0; k < jsonData.dropzone_left.length; k++) {
                        var point = jsonData.dropzone_left[k];
                        dropzoneLeftCalib[point.row][point.col] = {x: point.x, y: point.y};
                    }
                }
            }
        }
    }

    function getReversedRow(row) {
        return 7 - row;
    }

    function getReversedCol(zone, col) {
        if (zone === 0) { // chessboard
            return 7 - col;
        } else { // dropzones
            return 1 - col;
        }
    }

    function getCurrentPoint() {
        var actualRow = getReversedRow(selectedRow);
        var actualCol = getReversedCol(selectedZone, selectedCol);
        if (selectedZone === 0) {
            return chessboardCalib[actualRow] && chessboardCalib[actualRow][actualCol] ? chessboardCalib[actualRow][actualCol] : {x: 0, y: 0};
        } else if (selectedZone === 1) {
            return dropzoneRightCalib[actualRow] && dropzoneRightCalib[actualRow][actualCol] ? dropzoneRightCalib[actualRow][actualCol] : {x: 0, y: 0};
        } else {
            return dropzoneLeftCalib[actualRow] && dropzoneLeftCalib[actualRow][actualCol] ? dropzoneLeftCalib[actualRow][actualCol] : {x: 0, y: 0};
        }
    }

    function updatePopupValues() {
        var point = getCurrentPoint();
        popupX = point.x;
        popupY = point.y;
    }

    function adjustCurrentPoint(dx, dy) {
        var actualRow = getReversedRow(selectedRow);
        var actualCol = getReversedCol(selectedZone, selectedCol);
        var point;
        if (selectedZone === 0) {
            if (!chessboardCalib[actualRow] || !chessboardCalib[actualRow][actualCol]) return;
            point = chessboardCalib[actualRow][actualCol];
        } else if (selectedZone === 1) {
            if (!dropzoneRightCalib[actualRow] || !dropzoneRightCalib[actualRow][actualCol]) return;
            point = dropzoneRightCalib[actualRow][actualCol];
        } else {
            if (!dropzoneLeftCalib[actualRow] || !dropzoneLeftCalib[actualRow][actualCol]) return;
            point = dropzoneLeftCalib[actualRow][actualCol];
        }
        point.x += dx;
        point.y += dy;
        popupX = point.x;
        popupY = point.y;
        masterBot.updateCalibrationData(selectedZone,actualRow,actualCol,point.x,point.y);
    }

    function isCellExcluded(excludedCellsArray, actualRow, actualCol) {
        if (!excludedCellsArray || excludedCellsArray.length === 0) {
            return false; // If array is empty, no cells are excluded
        }
        for (var i = 0; i < excludedCellsArray.length; i++) {
            if (excludedCellsArray[i].actualRow === actualRow && excludedCellsArray[i].actualCol === actualCol) {
                return true;
            }
        }
        return false;
    }

    function isValidDropzoneLeft(row, col) {
        var actualRow = getReversedRow(row);
        var actualCol = getReversedCol(2, col);
        return actualRow >= 0 && actualRow < 8 && actualCol >= 0 && actualCol < 2 && !isCellExcluded(excludedLeftDropzoneCells, actualRow, actualCol);
    }

    function isValidDropzoneRight(row, col) {
        var actualRow = getReversedRow(row);
        var actualCol = getReversedCol(1, col);
        return actualRow >= 0 && actualRow < 8 && actualCol >= 0 && actualCol < 2 && !isCellExcluded(excludedRightDropzoneCells, actualRow, actualCol);
    }

    function isValidSelection(zone, row, col) {
        if (zone === 0) {
            return row >= 0 && row < 8 && col >= 0 && col < 8;
        } else if (zone === 1) {
            return isValidDropzoneRight(row, col);
        } else if (zone === 2) {
            return isValidDropzoneLeft(row, col);
        }
        return false;
    }

    function clampSelection() {
        if (selectedZone === 0) {
            selectedRow = Math.max(0, Math.min(selectedRow, 7));
            selectedCol = Math.max(0, Math.min(selectedCol, 7));
        } else if (selectedZone === 1) {
            selectedRow = Math.max(0, Math.min(selectedRow, 7));
            selectedCol = Math.max(0, Math.min(selectedCol, 1));
            if (!isValidDropzoneRight(selectedRow, selectedCol)) {
                // Try to find a valid position in the right dropzone
                if (selectedCol === 0 && isValidDropzoneRight(selectedRow, 1)) {
                    selectedCol = 1;
                } else if (selectedCol === 1 && isValidDropzoneRight(selectedRow, 0)) {
                    selectedCol = 0;
                } else {
                    // If no valid position in current row, move to a valid row
                    for (var r = 0; r < 8; r++) {
                        if (isValidDropzoneRight(r, selectedCol)) {
                            selectedRow = r;
                            break;
                        }
                    }
                }
            }
        } else {
            selectedRow = Math.max(0, Math.min(selectedRow, 7));
            selectedCol = Math.max(0, Math.min(selectedCol, 1));
            if (!isValidDropzoneLeft(selectedRow, selectedCol)) {
                selectedCol = 1;
            }
        }
    }

    property bool infoPopupVisible: false
    property bool confirmDialogVisible: false
    property int confirmButtonSelected: 0
    property int highlightDirection: 0 // 0:none, 1:up, 2:right, 3:down, 4:left
    property real popupX: 0
    property real popupY: 0
    
    // Upload progress properties
    property bool uploadProgressVisible: false
    property real uploadProgress: 0.0
    property bool uploadAbortDialogVisible: false
    property int uploadAbortButtonSelected: 0

    function abortCalibrationUpload() {
        // Send abort command to RobotController
        masterBot.abortCalibrationUpload();
        uploadProgressVisible = false;
        uploadAbortDialogVisible = false;
        // Return to parent panel
        root.exitPressed();
    }

    function startCalibrationUpload() {
        uploadProgress = 0.0;
        uploadProgressVisible = true;
        // Trigger the asynchronous upload process
        masterBot.sendCalibrationCells();
    }

    function simulateUploadProgress() {
        if (uploadProgress < 100) {
            uploadProgress += 2; // Simulate 2% progress per step
            if (uploadProgress >= 100) {
                uploadProgress = 100;
                // Upload complete - hide progress bar after a delay
                Qt.timer(function() {
                    uploadProgressVisible = false;
                }, 1000);
            } else {
                // Continue progress simulation
                Qt.timer(simulateUploadProgress, 100);
            }
        }
    }

    function navigateSelection(key) {
        if (confirmDialogVisible || infoPopupVisible) return;

        if (selectedZone === 0) {
            if (key === Qt.Key_Left) {
                if (selectedCol > 0) selectedCol--;
                else { selectedZone = 1; selectedCol = 1; }
            } else if (key === Qt.Key_Right) {
                if (selectedCol < 7) selectedCol++;
                else { selectedZone = 2; selectedCol = 0; }
            } else if (key === Qt.Key_Up) {
                if (selectedRow > 0) selectedRow--;
            } else if (key === Qt.Key_Down) {
                if (selectedRow < 7) selectedRow++;
            }
        } else if (selectedZone === 1) {
            if (key === Qt.Key_Right) {
                if (selectedCol < 1) selectedCol++;
                else { selectedZone = 0; selectedCol = 0; }
            } else if (key === Qt.Key_Left) {
                if (selectedCol > 0) {
                    var targetRow = selectedRow;
                    var targetCol = selectedCol - 1;
                    if (isValidDropzoneRight(targetRow, targetCol)) selectedCol--;
                }
            } else if (key === Qt.Key_Up) {
                if (selectedRow > 0) {
                    var targetRow = selectedRow - 1;
                    if (isValidDropzoneRight(targetRow, selectedCol)) selectedRow--;
                }
            } else if (key === Qt.Key_Down) {
                if (selectedRow < 7) {
                    var targetRow = selectedRow + 1;
                    if (isValidDropzoneRight(targetRow, selectedCol)) selectedRow++;
                }
            }
        } else if (selectedZone === 2) {
            if (key === Qt.Key_Left) {
                if (selectedCol > 0) {
                    var targetRow = selectedRow;
                    var targetCol = selectedCol - 1;
                    if (isValidDropzoneLeft(targetRow, targetCol)) selectedCol--;
                }
                else { selectedZone = 0; selectedCol = 7; }
            } else if (key === Qt.Key_Right) {
                if (selectedCol < 1) selectedCol++;
            } else if (key === Qt.Key_Up) {
                if (selectedRow > 0) {
                    var targetRow = selectedRow - 1;
                    if (isValidDropzoneLeft(targetRow, selectedCol)) selectedRow--;
                }
            } else if (key === Qt.Key_Down) {
                if (selectedRow < 7) {
                    var targetRow = selectedRow + 1;
                    if (isValidDropzoneLeft(targetRow, selectedCol)) selectedRow++;
                }
            }
        }
        clampSelection();
    }

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

    function drawPopupIndicator(ctx, w, h) {
        ctx.reset();
        var cx = w / 2;
        var cy = h / 2;
        var radius = 40;

        ctx.fillStyle = "#181818";
        ctx.strokeStyle = "#999";
        ctx.lineWidth = 2;
        ctx.beginPath();
        ctx.arc(cx, cy, radius, 0, 2 * Math.PI);
        ctx.fill();
        ctx.stroke();

        function drawTriangle(direction, active) {
            var size = active ? 18 : 12;
            var color = active ? "#00ff00" : "#888";
            ctx.fillStyle = color;
            ctx.beginPath();
            if (direction === 1) {
                ctx.moveTo(cx - size, cy - radius - 15);
                ctx.lineTo(cx + size, cy - radius - 15);
                ctx.lineTo(cx, cy - radius + 5);
            } else if (direction === 2) {
                ctx.moveTo(cx + radius + 15, cy - size);
                ctx.lineTo(cx + radius + 15, cy + size);
                ctx.lineTo(cx + radius - 5, cy);
            } else if (direction === 3) {
                ctx.moveTo(cx - size, cy + radius + 15);
                ctx.lineTo(cx + size, cy + radius + 15);
                ctx.lineTo(cx, cy + radius - 5);
            } else if (direction === 4) {
                ctx.moveTo(cx - radius - 15, cy - size);
                ctx.lineTo(cx - radius - 15, cy + size);
                ctx.lineTo(cx - radius + 5, cy);
            }
            ctx.closePath();
            ctx.fill();
        }

        drawTriangle(1, highlightDirection === 1);
        drawTriangle(2, highlightDirection === 2);
        drawTriangle(3, highlightDirection === 3);
        drawTriangle(4, highlightDirection === 4);
    }

    Rectangle {
        id: rectChessBoard
        anchors.fill: parent
        radius: 20
        color: "#e8e8e8"
        border.color: "#aaa"; border.width: 1
        focus: true

        Keys.onPressed: {
            if (uploadAbortDialogVisible) {
                if (event.key === Qt.Key_Left || event.key === Qt.Key_Right) {
                    uploadAbortButtonSelected = 1 - uploadAbortButtonSelected;
                    event.accepted = true;
                } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                    if (uploadAbortButtonSelected === 0) {
                        // Continue
                        uploadAbortDialogVisible = false;
                    } else {
                        // Abort
                        uploadAbortDialogVisible = false;
                        abortCalibrationUpload();
                    }
                    event.accepted = true;
                } else if (event.key === Qt.Key_Escape) {
                    uploadAbortDialogVisible = false;
                    event.accepted = true;
                }
            } else if (uploadProgressVisible) {
                if (event.key === Qt.Key_Escape) {
                    uploadAbortDialogVisible = true;
                    uploadAbortButtonSelected = 0; // Default to Continue
                    event.accepted = true;
                }
            } else if (confirmDialogVisible) {
                if (event.key === Qt.Key_Left || event.key === Qt.Key_Right) {
                    confirmButtonSelected = 1 - confirmButtonSelected;
                    event.accepted = true;
                } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                    if (confirmButtonSelected === 0) {
                        masterBot.saveCalibrationData();;
                    }
                    confirmDialogVisible = false;
                    infoPopupVisible = false;
                    event.accepted = true;
                    root.exitPressed();
                } else if (event.key === Qt.Key_Escape) {
                    confirmDialogVisible = false;
                    event.accepted = true;
                }
            } else if (infoPopupVisible) {
                if (event.key === Qt.Key_Left) {
                    highlightDirection = 4;
                    adjustCurrentPoint(-adjustValue, 0);
                    event.accepted = true;
                } else if (event.key === Qt.Key_Right) {
                    highlightDirection = 2;
                    adjustCurrentPoint(adjustValue, 0);
                    event.accepted = true;
                } else if (event.key === Qt.Key_Up) {
                    highlightDirection = 1;
                    adjustCurrentPoint(0, -adjustValue);
                    event.accepted = true;
                } else if (event.key === Qt.Key_Down) {
                    highlightDirection = 3;
                    adjustCurrentPoint(0, adjustValue);
                    event.accepted = true;
                } else if (event.key === Qt.Key_Escape) {
                    infoPopupVisible = false;
                    highlightDirection = 0;
                    event.accepted = true;
                }
            } else {
                if (event.key === Qt.Key_Left || event.key === Qt.Key_Right || event.key === Qt.Key_Up || event.key === Qt.Key_Down) {
                    navigateSelection(event.key);
                    event.accepted = true;
                } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                    infoPopupVisible = true;
                    highlightDirection = 0;
                    updatePopupValues();
                    masterBot.sendTestCommand("tx"+popupX+"y"+popupY);
                    event.accepted = true;
                } else if (event.key === Qt.Key_Escape) {
                    confirmDialogVisible = true;
                    confirmButtonSelected = 0;
                    event.accepted = true;
                }
            }
        }

        Keys.onReleased: {
            if (infoPopupVisible && (event.key === Qt.Key_Left || event.key === Qt.Key_Right || event.key === Qt.Key_Up || event.key === Qt.Key_Down)) {
                highlightDirection = 0;
            }
        }

        // Main horizontal container
        RowLayout {
            anchors.fill: parent
            anchors.margins: 40
            spacing: 30

            // --- LEFT COLUMN: DROP ZONE RIGHT ---
            Item {
                width: 100; height: 400
                GridLayout {
                    anchors.fill: parent
                    columns: 2; rows: 8
                    columnSpacing: 0; rowSpacing: 0

                    Repeater {
                        model: 16
                        Rectangle {
                            width: 50
                            height: 50
                            property int rowIdx: Math.floor(index / 2)
                            property int colIdx: index % 2
                            property bool isInvalidCell: isCellExcluded(excludedRightDropzoneCells, getReversedRow(rowIdx), getReversedCol(1, colIdx))
                            property bool isSelected: selectedZone === 1 && selectedRow === rowIdx && selectedCol === colIdx
                            color: isSelected ? "#ff00ff" : isInvalidCell ? "#111111" : "#2a2a2a"
                            border.color: isSelected ? "#ffffff" : isInvalidCell ? "#444444" : "#666666"
                            border.width: isSelected ? 2 : 1

                            Text {
                                anchors.centerIn: parent
                                text: "C" + getReversedCol(1, colIdx) + ",R" + getReversedRow(rowIdx)
                                color: "#ffffff"
                                font.pixelSize: 10
                                font.bold: true
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    if (!isInvalidCell) {
                                        selectedZone = 1;
                                        selectedRow = rowIdx;
                                        selectedCol = colIdx;
                                    }
                                }
                            }
                        }
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
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            property int rowIdx: Math.floor(index / 8)
                            property int colIdx: index % 8
                            property bool isSelected: selectedZone === 0 && selectedRow === rowIdx && selectedCol === colIdx
                            property bool isBlack: (rowIdx + colIdx) % 2 === 1
                            color: isSelected ? "#00ff00" : (isBlack ? "#222222" : "#ffffff")
                            border.color: isSelected ? "#ffffff" : "#333333"
                            border.width: isSelected ? 2 : 1

                            Text {
                                anchors.centerIn: parent
                                text: "C" + getReversedCol(0, colIdx) + ",R" + getReversedRow(rowIdx)
                                color: isBlack ? "#eeeeee" : "#111111"
                                font.pixelSize: 10
                                font.bold: true
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    selectedZone = 0;
                                    selectedRow = rowIdx;
                                    selectedCol = colIdx;
                                }
                            }
                        }
                    }
                }
            }

            // --- RIGHT COLUMN: THE 2x8 DROP ZONE LEFT ---
            Item {
                width: 100
                height: 400

                GridLayout {
                    anchors.fill: parent
                    columns: 2; rows: 8
                    columnSpacing: 0; rowSpacing: 0

                    Repeater {
                        model: 16
                        Rectangle {
                            width: 50
                            height: 50
                            property int rowIdx: Math.floor(index / 2)
                            property int colIdx: index % 2
                            property bool isInvalidCell: isCellExcluded(excludedLeftDropzoneCells, getReversedRow(rowIdx), getReversedCol(2, colIdx))
                            property bool isSelected: selectedZone === 2 && selectedRow === rowIdx && selectedCol === colIdx
                            color: isSelected ? "#ffaa00" : isInvalidCell ? "#111111" : "#2a2a2a"
                            border.color: isSelected ? "#ffffff" : isInvalidCell ? "#444444" : "#666666"
                            border.width: isSelected ? 2 : 1

                            Text {
                                anchors.centerIn: parent
                                text: "C" + getReversedCol(2, colIdx) + ",R" + getReversedRow(rowIdx)
                                color: "#ffffff"
                                font.pixelSize: 10
                                font.bold: true
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    if (!isInvalidCell) {
                                        selectedZone = 2;
                                        selectedRow = rowIdx;
                                        selectedCol = colIdx;
                                    }
                                }
                            }
                        }
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
        Rectangle {
            anchors.fill: parent
            color: "#00000088"
            visible: infoPopupVisible || confirmDialogVisible
            z: 10

            // Info popup
            Rectangle {
                visible: infoPopupVisible
                anchors.centerIn: parent
                width: 380
                height: 320
                radius: 16
                color: "#212121"
                border.color: "#00ff00"
                border.width: 2
                z: 11

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 12

                    Text {
                        text: "Calibration Position"
                        font.pixelSize: 18
                        color: "#ffffff"
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                    }

                    Text {
                        text: "X: " + popupX.toFixed(1) + " mm"
                        color: "#dddddd"
                        font.pixelSize: 14
                    }
                    Text {
                        text: "Y: " + popupY.toFixed(1) + " mm"
                        color: "#dddddd"
                        font.pixelSize: 14
                    }

                    Canvas {
                        id: indicatorCanvas
                        width: parent.width
                        height: 180
                        Layout.alignment: Qt.AlignHCenter
                        onPaint: {
                            var ctx = getContext("2d");
                            drawPopupIndicator(ctx, width, height);
                        }
                    }

                    Text {
                        text: "Use arrow keys to light the matching direction. Press Esc to close."
                        color: "#aaaaaa"
                        font.pixelSize: 12
                        wrapMode: Text.WordWrap
                    }
                }

                Connections {
                    target: root
                    onHighlightDirectionChanged: indicatorCanvas.requestPaint()
                    onInfoPopupVisibleChanged: if (infoPopupVisible) indicatorCanvas.requestPaint()
                }

                Connections {
                    target: masterBot
                    onCalibrationUploadProgress: {
                        uploadProgress = progress;
                    }
                    onCalibrationUploadComplete: {
                        uploadProgressVisible = false;
                        if (success) {
                            // Show success message or handle completion
                            console.log("Calibration upload completed successfully");
                        } else {
                            // Show error message or handle failure
                            console.log("Calibration upload failed");
                        }
                    }
                }
            }

            // Confirm dialog
            Rectangle {
                visible: confirmDialogVisible
                anchors.centerIn: parent
                width: 420
                height: 180
                radius: 16
                color: "#232323"
                border.color: "#888"
                border.width: 1
                z: 11

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 18
                    spacing: 14

                    Text {
                        text: "Do you want to store calibration file?"
                        color: "#ffffff"
                        font.pixelSize: 16
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                    }

                    RowLayout {
                        Layout.alignment: Qt.AlignHCenter
                        spacing: 24

                        Rectangle {
                            width: 120; height: 50
                            radius: 10
                            color: confirmButtonSelected === 0 ? "#00aa00" : "#444444"
                            border.color: confirmButtonSelected === 0 ? "#ffffff" : "#888888"
                            border.width: 1

                            Text {
                                anchors.centerIn: parent
                                text: "OK"
                                color: "#ffffff"
                                font.pixelSize: 14
                                font.bold: true
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    confirmButtonSelected = 0;
                                    masterBot.saveCalibrationData();
                                    confirmDialogVisible = false;
                                    root.exitPressed();
                                }
                            }
                        }

                        Rectangle {
                            width: 120; height: 50
                            radius: 10
                            color: confirmButtonSelected === 1 ? "#aa0000" : "#444444"
                            border.color: confirmButtonSelected === 1 ? "#ffffff" : "#888888"
                            border.width: 1

                            Text {
                                anchors.centerIn: parent
                                text: "Cancel"
                                color: "#ffffff"
                                font.pixelSize: 14
                                font.bold: true
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    confirmButtonSelected = 1;
                                    confirmDialogVisible = false;
                                    root.exitPressed();
                                }
                            }
                        }
                    }
                }
            }
        }


    }
}
