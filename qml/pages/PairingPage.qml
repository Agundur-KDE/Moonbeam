import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami
import org.agundur.moonbeam

Kirigami.ScrollablePage {
    id: root
    title: qsTr("Pair a Device")

    readonly property var sunshine: SunshineController
    readonly property var credentials: SunshineCredentials

    Component.onCompleted: root.credentials.ensure()

    Connections {
        target: root.sunshine

        function onPairingSucceeded() {
            resultMessage.type = Kirigami.MessageType.Positive;
            resultMessage.text = qsTr("Paired! The device should now be able to stream.");
            resultMessage.visible = true;
            pinField.text = "";
        }

        function onPairingFailed(reason) {
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

        // Only shown the very first time, and only if Sunshine already had
        // web UI credentials Moonbeam didn't set itself - otherwise
        // credentials are generated and stored in KWallet automatically,
        // with nothing for the user to type here.
        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: root.credentials.state === SunshineCredentials.NeedsExistingPassword
            type: Kirigami.MessageType.Warning
            text: qsTr("Sunshine's web UI already has a password set for user \"%1\". Enter it once so Moonbeam can remember it securely.").arg(root.credentials.user)
        }

        Controls.TextField {
            id: existingPasswordField
            Layout.fillWidth: true
            visible: root.credentials.state === SunshineCredentials.NeedsExistingPassword
            placeholderText: qsTr("Existing Sunshine web UI password")
            echoMode: TextInput.Password
        }

        Controls.Button {
            Layout.alignment: Qt.AlignRight
            visible: root.credentials.state === SunshineCredentials.NeedsExistingPassword
            text: qsTr("Remember Password")
            enabled: existingPasswordField.text.length > 0
            onClicked: root.credentials.provideExisting(existingPasswordField.text)
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: root.credentials.state === SunshineCredentials.Failed
            type: Kirigami.MessageType.Error
            text: qsTr("Could not set up Sunshine credentials automatically.")
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
            enabled: pinField.text.length > 0 && root.credentials.state === SunshineCredentials.Ready
                && !root.sunshine.pairingInProgress
            onClicked: {
                resultMessage.visible = false;
                root.sunshine.pair(pinField.text,
                                    nameField.text.length > 0 ? nameField.text : qsTr("Unnamed device"),
                                    root.credentials.user,
                                    root.credentials.password);
            }
        }
    }
}
