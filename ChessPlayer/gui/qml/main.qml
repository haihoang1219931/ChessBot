import QtQuick.Window 2.2
import QtQuick.Controls 2.0
import QtQuick 2.12
import QtQuick.Layouts 1.12

ApplicationWindow {
    id: wroot
    visible: true
    width: 640
    height: 480
    title: qsTr("ChessPlayer")
    color: "gray"

    StackView {
        id: stack
        anchors.fill: parent
        // 1. MUST HAVE FOCUS TRUE
        focus: true

        // 2. Use initialItem instead of just nesting the item
//        initialItem: levelSelection

        // Force focus to the current item whenever the stack changes
        onCurrentItemChanged: if (currentItem) currentItem.forceActiveFocus()

        Component.onCompleted: {
            stack.push(levelSelection)
        }
    }
    Component {
        id: levelSelection
        LevelSelection {
            onItemSelected: {
                stack.pop()
                stack.push(sideSelection)
            }
        }
    }

    Component {
        id: sideSelection
        SideSelection{
            onGoback: {
                stack.pop()
                stack.push(levelSelection)
            }
        }
    }


}
