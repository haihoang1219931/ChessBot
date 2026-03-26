import QtQuick 2.0
import QtMultimedia 5.12
FocusScope {
    id: root
    width: 640; height: 480
    signal exitPressed()
    property string filePath: "trapezoid_data.json"
    // Trapezoid points: [Bottom-Left, Bottom-Right, Top-Right, Top-Left]
    property var points: [
        {"x": 150, "y": 350}, {"x": 450, "y": 350},
        {"x": 350, "y": 150}, {"x": 250, "y": 150}
    ]
    property int activeIndex: 0
    property bool isEditing: false
    focus: true
    // 1. Set up the camera
    Camera {
        id: camera
    }

    // 2. Set up the VideoOutput
    VideoOutput {
        id: videoOutput
        anchors.fill: parent
        source: camera
    }

    Component.onCompleted: {
        var data = fileio.read(filePath);
        if (data !== "") {
            points = JSON.parse(data);
            canvas.requestPaint();
        }
    }
    Canvas {
        id: canvas
        anchors.fill: parent
        focus: true
        onPaint: {
            var ctx = getContext("2d");
            ctx.reset();

            // 1. Draw Trapezoid
            ctx.beginPath();
            ctx.moveTo(points[0].x, points[0].y);
            for (var i = 1; i < 4; i++) ctx.lineTo(points[i].x, points[i].y);
            ctx.closePath();
            ctx.fillStyle = "#330000FF";
            ctx.fill();
            ctx.strokeStyle = "blue";
            ctx.lineWidth = 2;
            ctx.stroke();

            // 2. Draw Corners
            for (var j = 0; j < 4; j++) {
                var isActive = (j === activeIndex);
                var radius = isActive ? 12 : 6; // Bigger if selected

                ctx.beginPath();
                ctx.arc(points[j].x, points[j].y, radius, 0, 2 * Math.PI);

                // Color logic: Red if editing, Yellow if just selected, White otherwise
                ctx.fillStyle = isActive ? (isEditing ? "red" : "yellow") : "white";

                ctx.fill();
//                ctx.strokeStyle = "black";
//                ctx.stroke();
            }
        }

        Keys.onPressed: (event) => {
            var step = 2;
            if (event.key === Qt.Key_Space) {
                fileio.write(filePath, JSON.stringify(points));
                console.log("Points saved to " + filePath);
            }
            else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                isEditing = !isEditing; // Toggle mode
            }
            else if (isEditing) {
                // Move Mode
                if (event.key === Qt.Key_Left)  points[activeIndex].x -= step;
                if (event.key === Qt.Key_Right) points[activeIndex].x += step;
                if (event.key === Qt.Key_Up)    points[activeIndex].y -= step;
                if (event.key === Qt.Key_Down)  points[activeIndex].y += step;
            }
            else {
                // Navigation Mode
                if (event.key === Qt.Key_Left || event.key === Qt.Key_Down)
                    activeIndex = (activeIndex + 1) % 4;
                if (event.key === Qt.Key_Right || event.key === Qt.Key_Up)
                    activeIndex = ((activeIndex >= 1? activeIndex:activeIndex+4) - 1) % 4;
            }

            canvas.requestPaint();
        }
        Keys.onEscapePressed: {
            root.exitPressed()
        }
    }
}
