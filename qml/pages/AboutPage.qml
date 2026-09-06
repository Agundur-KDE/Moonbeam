import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

// Custom About page, not Kirigami.AboutPage: that component's "Donate" and
// "Get Involved" buttons read their own AboutItem-internal defaults, which
// are silently gated on KAboutData's desktopFileName looking like
// "org.kde.*" - not on anything KAboutData::setUrl() controls - so a
// third-party app's links were always overridden by KDE's own community
// pages regardless of what main.cpp configured. Its "Report a bug" button
// also unconditionally opens bugs.kde.org afterward regardless of
// bugAddress, due to a missing return/else in Kirigami's own AboutItem.qml.
// For "just a few links", it was simpler and more reliable to own this
// page outright than to fight those framework internals - same spirit as
// KCast's plasmoid metadata (de.agundur.kcast), which likewise points its
// own Website/BugReportUrl fields at Agundur's own infrastructure rather
// than any KDE default.
Kirigami.ScrollablePage {
    id: root
    title: i18n("About Moonbeam")

    ColumnLayout {
        width: parent.width
        spacing: Kirigami.Units.largeSpacing

        Kirigami.Icon {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: Kirigami.Units.iconSizes.huge
            Layout.preferredHeight: Kirigami.Units.iconSizes.huge
            source: "org.agundur.moonbeam"
        }

        Kirigami.Heading {
            Layout.alignment: Qt.AlignHCenter
            text: i18n("Moonbeam 0.1.0")
            level: 2
        }

        Controls.Label {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            text: i18n("Share your desktop over the network via Sunshine/Moonlight")
            opacity: 0.8
        }

        Controls.Label {
            Layout.alignment: Qt.AlignHCenter
            text: i18n("© 2026 Agundur — GPL v3")
            opacity: 0.6
        }

        Kirigami.Separator {
            Layout.fillWidth: true
            Layout.topMargin: Kirigami.Units.largeSpacing
        }

        Controls.Button {
            Layout.fillWidth: true
            flat: true
            icon.name: "globe-symbolic"
            text: i18n("Homepage")
            onClicked: Qt.openUrlExternally("https://github.com/Agundur-KDE/Moonbeam")
        }

        Controls.Button {
            Layout.fillWidth: true
            flat: true
            icon.name: "donate-symbolic"
            text: i18n("Donate")
            onClicked: Qt.openUrlExternally("https://github.com/sponsors/Agundur-KDE")
        }

        Controls.Button {
            Layout.fillWidth: true
            flat: true
            icon.name: "applications-development-symbolic"
            text: i18n("Get Involved")
            onClicked: Qt.openUrlExternally("https://github.com/Agundur-KDE/Moonbeam/discussions")
        }

        Controls.Button {
            Layout.fillWidth: true
            flat: true
            icon.name: "tools-report-bug-symbolic"
            text: i18n("Report a Bug")
            onClicked: Qt.openUrlExternally("https://github.com/Agundur-KDE/Moonbeam/issues")
        }

        // Separate from "Report a Bug": matches the de.agundur.kcast
        // plasmoid's own BugReportUrl convention
        // ("mailto:info@agundur.de?subject=KCast%20bug%20report") for
        // reaching Agundur directly, not routed through GitHub at all.
        Controls.Button {
            Layout.fillWidth: true
            flat: true
            icon.name: "mail-message-new-symbolic"
            text: i18n("Contact")
            onClicked: Qt.openUrlExternally("mailto:info@agundur.de?subject=Moonbeam%20feedback")
        }
    }
}
