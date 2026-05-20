import QtQuick 2.0
import QtQuick.Controls 2.0
import QtGraphicalEffects 1.0
import QtQml 2.0

Item {
    id: root
    width: 800
    height: 480

    signal overlayHidden()

    property int direction: 0
    property var titleMap: ["Init communication",
        "Upload calib to robot",
        "Request calib from robot",
        "Enable robot",
        "Homming"
    ]
    property int progress: 0
    property bool finished: false
    property bool success: false

//    ShaderEffectSource {
//        id: source
//        anchors.fill: parent
//        sourceItem: parent
//        live: true
//        recursive: true
//        hideSource: true
//    }

//    FastBlur {
//        anchors.fill: parent
//        source: source
//        radius: 18
//    }

    Rectangle {
        anchors.fill: parent
        color: "black"
        opacity: 0.35
    }

    Rectangle {
        id: card
        width: Math.min(parent.width * 0.8, 520)
        height: 96
        radius: 10
        color: "transparent"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter

        Column {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 8
            anchors.horizontalCenter: parent.horizontalCenter
            Text {
                id: statusText
                text: root.titleMap[root.direction] +
                      (root.finished ? (root.success ? " complete" : " failed") : "...")
                color: "white"
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: 14
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Rectangle {
                id: progressBar
                width: card.width * 0.98
                height: 50
                color: "white"
                opacity: 0.95
                Rectangle {
                    width: progressBar.width*root.progress/100
                    height: parent.height
                    radius: 6
                    color: root.finished ? (root.success ? "#2CFF05" : "#FF3300") : "#00ccff"
                }
            }
        }
    }

    Timer {
        id: hideTimer
        interval: 3000
        repeat: false
        onTriggered: {
            root.progress = 0
            root.finished = false
            root.success = false
            root.overlayHidden()
        }
    }

    function show() {
        root.finished = false
        root.success = false
        root.progress = 0
    }

    Connections {
        target: backend
        onCalibrationUploadProgress: function(direction, progress){
            root.show()
            root.progress = progress
            root.direction = direction
        }
        onCalibrationUploadComplete: function(direction, success) {
            root.finished = true
            root.success = success
            if (success) root.progress = 100
            hideTimer.start()
            root.direction = direction
        }
    }

    Component.onCompleted: show()
}
