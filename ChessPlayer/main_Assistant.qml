 import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

ApplicationWindow {
    visible: true
    width: 500
    height: 600
    title: "Chess AI Voice Assistant"

    // Custom dark-themed background color
    background: Rectangle {
        color: "#1e1e1e"
    }

    // Instantiate your C++ Assistant Controller
    Connections {
        target: assistant
        onGenerationFinished: function(finalText) {
            console.log("Generation completed successfully!")
            // You can trigger your Text-to-Speech logic here later
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 15

        // 1. Output Display: Scrollable Text Area
        GroupBox {
            title: "Assistant Commentary"
            Layout.fillWidth: true
            Layout.fillHeight: true

            label: Text {
                text: parent.title
                color: "#b0b0b0"
                font.bold: true
            }

            background: Rectangle {
                color: "#2d2d2d"
                border.color: "#3d3d3d"
                radius: 6
            }

            ScrollView {
                anchors.fill: parent
                clip: true

                TextField {
                    id: txtAssistant
                    text: assistant.responseText
                    wrapMode: Text.Wrap
                    color: "#ffffff"
                    font.pointSize: 13
                    placeholderText: "Your chess analysis will appear here..."
                    placeholderTextColor: "#707070"

                    background: null // Transparent background inside the GroupBox frame
                }
            }
        }

        // 2. Input Field: Text Field for prompts or FEN strings
        TextField {
            id: inputField
            Layout.fillWidth: true
            placeholderText: "Type a prompt or paste a FEN string here..."
            color: "#ffffff"
            font.pointSize: 12
            selectByMouse: true

            // Allow pressing "Enter" on the keyboard to submit the request
            onAccepted: {
                if (inputField.text.trim() !== "" && !assistant.isThinking) {
                    assistant.generateResponse(inputField.text)
                }
            }

            background: Rectangle {
                color: "#2d2d2d"
                border.color: inputField.activeFocus ? "#2196F3" : "#3d3d3d"
                border.width: inputField.activeFocus ? 2 : 1
                radius: 6
            }
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 15
            Button {
                id: micButton
                text: assistant.isListening ? "🛑 Stop Listening" : "🎤 Talk to Assistant"
                Layout.preferredHeight: 50
                Layout.fillWidth: true

                onClicked: {
                    console.log("micButton "+assistant.isListening);
                    if (assistant.isListening) {
                        assistant.stopListening()
                    } else {
                        assistant.startListening()
                    }
                }

                background: Rectangle {
                    // Flashes red when recording audio from Windows hardware
                    color: assistant.isListening ? "#d32f2f" : (micButton.enabled ? "#4CAF50" : "#444444")
                    radius: 8
                }
            }
            // 3. Action Trigger: Button to start request
            Button {
                id: actionButton
                Layout.fillWidth: true
                Layout.preferredHeight: 45

                // Dynamic text changes depending on the C++ backend state
                text: assistant.isThinking ? "Thinking..." : "Request"
                enabled: inputField.text.trim() !== "" && !assistant.isThinking

                onClicked: {
                    console.log("generateResponse");
                    assistant.generateResponse(inputField.text)
                }

                // Custom modern visual styling for the button
                contentItem: Text {
                    text: actionButton.text
                    font.pointSize: 12
                    font.bold: true
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    color: !actionButton.enabled ? "#444444" : (actionButton.down ? "#1976D2" : "#2196F3")
                    radius: 6
                }
            }
            Button {
                id: testButton
                text: "Test Voice"
                Layout.fillWidth: true
                Layout.preferredHeight: 45
                onClicked: {
                    assistant.singleVoice(txtAssistant.text);
                }
            }
        }
    }
    Component.onCompleted: {
        assistant.startService()
    }
}
