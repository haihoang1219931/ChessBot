import QtQuick 2.0
import QtQuick.Controls 2.0
Row {
    id: root
    property string title
    property alias armLength: spbArmWidth.value
    property alias angle: sldArmAngle.value
    property alias editAngle: cbxAngleChangeEnable.checked
    spacing: 10

    Label {
        width: 40
        height: 40
        text: root.title
        verticalAlignment: Text.AlignVCenter
    }
    SpinBox {
        id: spbArmWidth
        from: 0
        to: 1000
    }
    Slider {
        id: sldArmAngle
        width: 150
        height: 40
        from: -180
        to: 360
        enabled: cbxAngleChangeEnable.checked
    }
    Label {
        width: 40
        height: 40
        text: Number(sldArmAngle.value).toFixed(2)
        verticalAlignment: Text.AlignVCenter
    }
    CheckBox {
        id: cbxAngleChangeEnable
        width: 40
        height: 40
        text: "Edit angle"
        checked: true
    }
}
