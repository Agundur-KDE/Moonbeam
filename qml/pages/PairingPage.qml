import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami
import org.agundur.moonbeam

Kirigami.ScrollablePage {
    title: qsTr("Pair a Device")

    SunshineController {
        id: sunshine

        onPairingSucceeded: {
            resultMessage.type = Kirigami.MessageType.Positive;
            resultMessage.text = qsTr("Paired! The device should now be able to stream.");
            resultMessage.visible = true;
            pinField.text = "";
        }
        onPairingFailed: reason => {
            resultMessage.type = Kirigami.MessageType.Error;
            resultMessage.text = qsTr("Pairing failed: %1").arg(reason);
            resultMessage.visible = true;
        }
    }

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
            id: webUiUserField
            Layout.fillWidth: true
            placeholderText: qsTr("Sunshine web UI username")
            enabled: !sunshine.pairingInProgress
        }

        Controls.TextField {
            id: webUiPasswordField
            Layout.fillWidth: true
            placeholderText: qsTr("Sunshine web UI password")
            echoMode: TextInput.Password
            enabled: !sunshine.pairingInProgress
        }

        Kirigami.InlineMessage {
            id: resultMessage
            Layout.fillWidth: true
            visible: false
        }

        Controls.TextField {
            id: pinField
            Layout.fillWidth: true
            placeholderText: qsTr("PIN from Moonlight")
            enabled: !sunshine.pairingInProgress
            validator: IntValidator { bottom: 0; top: 9999 }
        }

        Controls.TextField {
            id: nameField
            Layout.fillWidth: true
            placeholderText: qsTr("Device name (optional)")
            enabled: !sunshine.pairingInProgress
        }

        Controls.Button {
            Layout.alignment: Qt.AlignRight
            text: sunshine.pairingInProgress ? qsTr("Pairing…") : qsTr("Pair")
            icon.name: "dialog-ok"
            enabled: pinField.text.length > 0 && webUiUserField.text.length > 0
                && webUiPasswordField.text.length > 0 && !sunshine.pairingInProgress
            onClicked: {
                resultMessage.visible = false;
                sunshine.pair(pinField.text,
                              nameField.text.length > 0 ? nameField.text : qsTr("Unnamed device"),
                              webUiUserField.text,
                              webUiPasswordField.text);
            }
        }
    }
}
