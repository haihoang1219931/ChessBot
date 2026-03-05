import QtQuick 2.12

FocusScope {
    id: robotFace
    width: 640; height: 480
    focus: true
    property var listEmotion: ["neutral", "happy", "sad", "angry", "surprise"]
    property int emotionID: 0
    property string emotion: robotFace.listEmotion[emotionID]
    signal enterPressed()
    Keys.onLeftPressed: {
        console.log("Left btn pressed");
        emotionID++;
        emotionID = emotionID % 5;
    }

    Keys.onEnterPressed: {
        console.log("Enter Pressed");
        robotFace.enterPressed();
    }

    // Eye components
    Row {
        anchors.centerIn: parent
        spacing: 60
        Rectangle {
            id: leftEye
            width: 70; height: 70; radius: 35
            color: "transparent"; border.width: 6; border.color: "#A0E0FF"
        }
        Rectangle {
            id: rightEye
            width: 70; height: 70; radius: 35
            color: "transparent"; border.width: 6; border.color: "#A0E0FF"
        }
    }

    // Mouth component
    Rectangle {
        id: mouth
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom; anchors.bottomMargin: 150
        width: 40; height: 10; radius: 5
        color: "transparent"; border.width: 4; border.color: "#A0E0FF"
    }

    states: [
        State {
            name: "happy"
            when: robotFace.emotion === "happy"
            PropertyChanges { target: mouth; height: 30; radius: 15; anchors.bottomMargin: 140 }
            PropertyChanges { target: robotFace; opacity: 1.0 }
        },
        State {
            name: "sad"
            when: robotFace.emotion === "sad"
            PropertyChanges { target: mouth; width: 40; height: 15; radius: 5; rotation: 180 }
            PropertyChanges { target: leftEye; height: 40 } // Droopy eyes
            PropertyChanges { target: rightEye; height: 40 } // Droopy eyes
        },
        State {
            name: "angry"
            when: robotFace.emotion === "angry"
            PropertyChanges { target: mouth; width: 20; height: 5; border.color: "red" }
            PropertyChanges { target: leftEye; border.color: "red"; rotation: 15 }
            PropertyChanges { target: rightEye; border.color: "red"; rotation: 15 }
        },
        State {
            name: "surprise"
            when: robotFace.emotion === "surprise"
            PropertyChanges { target: leftEye; width: 100; height: 100; radius: 50 }
            PropertyChanges { target: rightEye; width: 100; height: 100; radius: 50 }
            PropertyChanges { target: mouth; width: 40; height: 40; radius: 20 }
        }
    ]

    transitions: Transition {
        NumberAnimation { properties: "width,height,radius,rotation,opacity"; duration: 300; easing.type: Easing.InOutQuad }
        ColorAnimation { duration: 300 }
    }

    // Cycle through emotions on click
    MouseArea {
        anchors.fill: parent
        onClicked: {
            let emotions = ["neutral", "happy", "sad", "angry", "surprise"]
            let nextIndex = (emotions.indexOf(robotFace.emotion) + 1) % emotions.length
            robotFace.emotion = emotions[nextIndex]
        }
    }
}
