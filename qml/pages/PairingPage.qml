import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami
import org.agundur.moonbeam

Kirigami.ScrollablePage {
    id: root
    title: qsTr("Pair a Device")

    readonly property var sunshine: SunshineController

    Connections {
        target: root.sunshine

        function onPairingSucceeded() {
            resultMessage.type = Kirigami.MessageType.Positive;
            resultMessage.text = qsTr("Paired! The device should now be able to stream.");
            resultMessage.visible = true;
            pinField.text = "";
            webUiPasswordField.text = "";
        }

        function onPairingFailed(reason) {
            resultMessage.type = Kirigami.MessageType.Error;
            resultMessage.text = qsTr("Pairing failed: %1").arg(reason);
            resultMessage.visible = true;
            webUiPasswordField.text = "";
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
            enabled: !root.sunshine.pairingInProgress
        }

        Controls.TextField {
            id: webUiPasswordField
            Layout.fillWidth: true
            placeholderText: qsTr("Sunshine web UI password")
            echoMode: TextInput.Password
            enabled: !root.sunshine.pairingInProgress
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
            enabled: !root.sunshine.pairingInProgress
            validator: IntValidator { bottom: 0; top: 9999 }
        }

        Controls.TextField {
            id: nameField
            Layout.fillWidth: true
            placeholderText: qsTr("Device name (optional)")
            enabled: !root.sunshine.pairingInProgress
        }

        Controls.Button {
            Layout.alignment: Qt.AlignRight
            text: root.sunshine.pairingInProgress ? qsTr("Pairing…") : qsTr("Pair")
            icon.name: "dialog-ok"
            enabled: pinField.text.length > 0 && webUiUserField.text.length > 0
                && webUiPasswordField.text.length > 0 && !root.sunshine.pairingInProgress
            onClicked: {
                resultMessage.visible = false;
                root.sunshine.pair(pinField.text,
                                    nameField.text.length > 0 ? nameField.text : qsTr("Unnamed device"),
                                    webUiUserField.text,
                                    webUiPasswordField.text);
            }
        }
    }
}
