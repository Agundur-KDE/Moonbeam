import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami
import org.kde.coreaddons as CoreAddons
import org.agundur.moonbeam

Kirigami.ScrollablePage {
    id: root
    title: i18n("Share Desktop")

    readonly property var sunshine: SunshineController
    readonly property var credentials: SunshineCredentials
    readonly property bool isSharing: root.sunshine.state === SunshineController.RunningOwned
        || root.sunshine.state === SunshineController.RunningExternal

    // So the Web UI login fields below are already populated by the time
    // "Open Web UI" is clickable, without requiring a detour through the
    // Pairing page first.
    Component.onCompleted: root.credentials.ensure()

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

    actions: [
        Kirigami.Action {
            text: i18n("About Moonbeam")
            icon.name: "help-about"
            onTriggered: applicationWindow().pageStack.push(aboutPage)
        }
    ]

    Kirigami.AboutPage {
        id: aboutPage
        visible: false
        aboutData: CoreAddons.AboutData
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
            text: i18n("Sunshine isn't installed. Install it with your package manager, e.g. `sudo zypper install sunshine` on openSUSE.")
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: root.sunshine.state === SunshineController.ConfigInvalid
            type: Kirigami.MessageType.Error
            text: i18n("Sunshine's configured port in sunshine.conf is invalid, so Moonbeam can't safely check or start it. Fix the `port` value there first.")
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: root.sunshine.state === SunshineController.RunningExternal
            type: Kirigami.MessageType.Information
            text: i18n("An existing Sunshine instance is already running - Moonbeam is using it, not starting a second one.")
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: root.isSharing
            type: Kirigami.MessageType.Information
            text: i18n("Connect from Moonlight on your TV, tablet, or phone. First time on a new device? You'll need a pairing PIN.")
        }

        // A paired Moonlight client gets full mouse/keyboard/controller
        // control of this PC by default (Sunshine emulates real input
        // devices) - not just a screen view. Shown whenever that's
        // currently the case, so it's never a silent surprise for
        // presenting/screen-sharing use cases.
        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: !root.sunshine.viewOnly
            type: Kirigami.MessageType.Warning
            text: i18n("Anyone who pairs a device gets full mouse, keyboard, and controller control of this PC - not just a screen view.")
        }

        Controls.CheckBox {
            Layout.alignment: Qt.AlignHCenter
            text: i18n("View only (no mouse/keyboard/controller control)")
            checked: root.sunshine.viewOnly
            enabled: root.sunshine.canStart
            onToggled: root.sunshine.viewOnly = checked
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: Kirigami.Units.largeSpacing

            Controls.Button {
                text: root.sunshine.canStop ? i18n("Stop Sharing") : i18n("Start Sharing")
                icon.name: root.sunshine.canStop ? "media-playback-stop" : "media-playback-start"
                enabled: root.sunshine.canStart || root.sunshine.canStop
                onClicked: root.sunshine.canStop ? root.sunshine.stop() : root.sunshine.start()
            }

            Controls.Button {
                text: i18n("Pair a Device")
                icon.name: "network-connect"
                enabled: root.isSharing
                onClicked: applicationWindow().pageStack.push(Qt.resolvedUrl("PairingPage.qml"))
            }

            Controls.Button {
                text: i18n("Open Web UI")
                icon.name: "internet-web-browser"
                enabled: root.isSharing
                onClicked: Qt.openUrlExternally(root.sunshine.webUiUrl)
            }
        }

        // Sunshine's web UI asks for these same credentials via the
        // browser's own Basic-Auth prompt - shown here (masked, copy
        // buttons only) so the user has something to paste into it rather
        // than having to dig them out of KWallet by hand.
        ColumnLayout {
            Layout.alignment: Qt.AlignHCenter
            visible: root.isSharing && root.credentials.state === SunshineCredentials.Ready
            spacing: Kirigami.Units.smallSpacing

            Kirigami.Heading {
                Layout.alignment: Qt.AlignHCenter
                text: i18n("Web UI Login")
                level: 4
            }

            RowLayout {
                Controls.Label {
                    text: i18n("Username:")
                }
                Controls.TextField {
                    id: webUiUserField
                    readOnly: true
                    text: root.credentials.user
                }
                Controls.Button {
                    icon.name: "edit-copy"
                    text: i18n("Copy")
                    onClicked: {
                        webUiUserField.selectAll();
                        webUiUserField.copy();
                    }
                }
            }

            RowLayout {
                Controls.Label {
                    text: i18n("Password:")
                }
                Controls.TextField {
                    id: webUiPasswordField
                    readOnly: true
                    echoMode: TextInput.Password
                    text: root.credentials.password
                }
                Controls.Button {
                    icon.name: "edit-copy"
                    text: i18n("Copy")
                    // TextInput.copy() silently refuses to copy anything
                    // while echoMode is Password (Qt's own anti-shoulder-
                    // surfing measure) - copying the password has to go
                    // through C++ directly instead.
                    onClicked: root.credentials.copyPasswordToClipboard()
                }
            }
        }
    }
}
