import QtQuick 2.0

RobotArm {
    id: root
    width: 200
    height: 40
    property real grabberAngle: 0
    property real dGrabberAngle: 0.1
    Rectangle {
        id: rect
        anchors.left: parent.right
        anchors.verticalCenter: parent.verticalCenter
        color: "green"
        width: root.height/2
        height: root.height*2
    }
    Rectangle {
        color: "red"
        x: root.height/2 + rect.x
        y: grabberAngle * root.height/2-root.height/2
        width: root.height/2
        height: root.height/2
    }
    Rectangle {
        color: "blue"
        x: root.height/2 + rect.x
        y: root.height*3/2 - grabberAngle * root.height/2-root.height/2
        width: root.height/2
        height: root.height/2
    }
//    Timer {
//        repeat: true
//        running: true
//        interval: 30
//        onTriggered: {
//            grabberAngle += dGrabberAngle;
//            if(grabberAngle < 0) dGrabberAngle = 0.1;
//            if(grabberAngle > 1) dGrabberAngle = -0.1;
//        }
//    }
}
