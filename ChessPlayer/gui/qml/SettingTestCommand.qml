import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12

FocusScope {
    id: root
    width: 800
    height: 480

    signal exitPressed()
    focus: true

    // Fallback escape handler for the root scope
    Keys.onEscapePressed: {
        root.exitPressed();
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 40
        spacing: 30

        TextField {
            id: txtCommand
            Layout.fillWidth: true
            Layout.preferredHeight: 40

            // Forces focus onto the text field immediately when the UI loads
            focus: true
            placeholderText: "Enter command here..."

            // KEYBOARD FIX: Intercept Escape key while typing
            Keys.onEscapePressed: {
                root.exitPressed();
                event.accepted = true; // Prevents the event from bubbling up further
            }

            // KEYBOARD FIX: Pressing Enter/Return sends the command automatically
            onAccepted: {
                masterBot.sendTestCommand(txtCommand.text);
            }
        }

        Button {
            id: btnSend
            text: "Send"
            Layout.preferredWidth: 100
            Layout.preferredHeight: 40

            // Button remains active, but keyboard users can just press Enter in the text field
            onClicked: {
                masterBot.sendTestCommand(txtCommand.text);
            }
        }
    }
}
