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

  ColumnLayout {
    width: parent.width
    spacing: Kirigami.Units.largeSpacing

    Kirigami.Icon {
      Layout.alignment: Qt.AlignHCenter
      source: sunshine.running ? "video-display" : "video-display-off"
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
      visible: sunshine.running
      type: Kirigami.MessageType.Information
      text: qsTr("Connect from Moonlight on your TV, tablet, or phone. First time on a new device? You'll need a pairing PIN.")
    }

    RowLayout {
      Layout.alignment: Qt.AlignHCenter
      spacing: Kirigami.Units.largeSpacing

      Controls.Button {
        text: sunshine.running ? qsTr("Stop Sharing") : qsTr("Start Sharing")
        icon.name: sunshine.running ? "media-playback-stop" : "media-playback-start"
        onClicked: sunshine.running ? sunshine.stop() : sunshine.start()
      }

      Controls.Button {
        text: qsTr("Pair a Device")
        icon.name: "network-connect"
        enabled: sunshine.running
        onClicked: applicationWindow().pageStack.push(Qt.resolvedUrl("PairingPage.qml"))
      }
    }
  }
}
