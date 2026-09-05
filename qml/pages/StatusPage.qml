import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami
import org.agundur.moonbeam

Kirigami.ScrollablePage {
    title: qsTr("Share Desktop")

    SunshineController {
        id: sunshine
    }

    readonly property bool isSharing: sunshine.state === SunshineController.RunningOwned
        || sunshine.state === SunshineController.RunningExternal

    function iconFor(state) {
        switch (state) {
        case SunshineController.RunningExternal:
        case SunshineController.RunningOwned:
            return "video-display";
        case SunshineController.NotInstalled:
            return "dialog-warning";
        default:
            return "video-display-off";
        }
    }

    ColumnLayout {
        width: parent.width
        spacing: Kirigami.Units.largeSpacing

        Kirigami.Icon {
            Layout.alignment: Qt.AlignHCenter
            source: iconFor(sunshine.state)
            Layout.preferredWidth: Kirigami.Units.iconSizes.huge
            Layout.preferredHeight: Kirigami.Units.iconSizes.huge
        }

        Kirigami.Heading {
            Layout.alignment: Qt.AlignHCenter
            text: sunshine.statusText
            level: 2
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: sunshine.state === SunshineController.NotInstalled
            type: Kirigami.MessageType.Warning
            text: qsTr("Sunshine isn't installed. Install it with your package manager, e.g. `sudo zypper install sunshine` on openSUSE.")
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: sunshine.state === SunshineController.RunningExternal
            type: Kirigami.MessageType.Information
            text: qsTr("An existing Sunshine instance is already running - Moonbeam is using it, not starting a second one.")
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: isSharing
            type: Kirigami.MessageType.Information
            text: qsTr("Connect from Moonlight on your TV, tablet, or phone. First time on a new device? You'll need a pairing PIN.")
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: Kirigami.Units.largeSpacing

            Controls.Button {
                text: sunshine.canStop ? qsTr("Stop Sharing") : qsTr("Start Sharing")
                icon.name: sunshine.canStop ? "media-playback-stop" : "media-playback-start"
                enabled: sunshine.canStart || sunshine.canStop
                onClicked: sunshine.canStop ? sunshine.stop() : sunshine.start()
            }

            Controls.Button {
                text: qsTr("Pair a Device")
                icon.name: "network-connect"
                enabled: isSharing
                onClicked: applicationWindow().pageStack.push(Qt.resolvedUrl("PairingPage.qml"))
            }
        }
    }
}
