import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami
import org.agundur.moonbeam

Kirigami.ScrollablePage {
    id: root
    title: qsTr("Share Desktop")

    readonly property var sunshine: SunshineController
    readonly property bool isSharing: root.sunshine.state === SunshineController.RunningOwned
        || root.sunshine.state === SunshineController.RunningExternal

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
            source: root.iconFor(root.sunshine.state)
            Layout.preferredWidth: Kirigami.Units.iconSizes.huge
            Layout.preferredHeight: Kirigami.Units.iconSizes.huge
        }

        Kirigami.Heading {
            Layout.alignment: Qt.AlignHCenter
            text: root.sunshine.statusText
            level: 2
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: root.sunshine.state === SunshineController.NotInstalled
            type: Kirigami.MessageType.Warning
            text: qsTr("Sunshine isn't installed. Install it with your package manager, e.g. `sudo zypper install sunshine` on openSUSE.")
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: root.sunshine.state === SunshineController.RunningExternal
            type: Kirigami.MessageType.Information
            text: qsTr("An existing Sunshine instance is already running - Moonbeam is using it, not starting a second one.")
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: root.isSharing
            type: Kirigami.MessageType.Information
            text: qsTr("Connect from Moonlight on your TV, tablet, or phone. First time on a new device? You'll need a pairing PIN.")
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: Kirigami.Units.largeSpacing

            Controls.Button {
                text: root.sunshine.canStop ? qsTr("Stop Sharing") : qsTr("Start Sharing")
                icon.name: root.sunshine.canStop ? "media-playback-stop" : "media-playback-start"
                enabled: root.sunshine.canStart || root.sunshine.canStop
                onClicked: root.sunshine.canStop ? root.sunshine.stop() : root.sunshine.start()
            }

            Controls.Button {
                text: qsTr("Pair a Device")
                icon.name: "network-connect"
                enabled: root.isSharing
                onClicked: applicationWindow().pageStack.push(Qt.resolvedUrl("PairingPage.qml"))
            }
        }
    }
}
