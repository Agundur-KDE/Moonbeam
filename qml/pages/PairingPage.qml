import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

Kirigami.ScrollablePage {
    title: qsTr("Pair a Device")

    ColumnLayout {
        width: parent.width
        spacing: Kirigami.Units.largeSpacing

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: true
            type: Kirigami.MessageType.Information
            text: qsTr("Open Moonlight on the device you want to connect, add this PC, then enter the PIN it shows you below.")
        }

        Controls.TextField {
            id: pinField
            Layout.fillWidth: true
            placeholderText: qsTr("PIN from Moonlight")
            validator: IntValidator { bottom: 0; top: 999999 }
        }

        Controls.Button {
            Layout.alignment: Qt.AlignRight
            text: qsTr("Pair")
            icon.name: "dialog-ok"
            enabled: pinField.text.length > 0
            onClicked: {
                // Sketch stage: needs to call into SunshineController /
                // sunshine's pairing endpoint with pinField.text.
            }
        }
    }
}
