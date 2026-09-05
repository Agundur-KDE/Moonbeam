import QtQuick
import org.kde.kirigami as Kirigami
import org.agundur.moonbeam

Kirigami.ApplicationWindow {
  id: root
  title: qsTr("Moonbeam")

  width: 480
  height: 640

  pageStack.initialPage: statusPageComponent

  globalDrawer: Kirigami.GlobalDrawer {
    title: qsTr("Moonbeam")
    titleIcon: "org.agundur.moonbeam"

    actions: [
      Kirigami.Action {
        text: qsTr("Share Desktop")
        icon.name: "video-display"
        onTriggered: root.pageStack.replace(statusPageComponent)
      },
      Kirigami.Action {
        text: qsTr("Cast Media")
        icon.name: "media-playback-start"
        onTriggered: root.pageStack.replace(mediaCastPageComponent)
      }
    ]
  }

  Component {
    id: statusPageComponent
    StatusPage {}
  }

  Component {
    id: pairingPageComponent
    PairingPage {}
  }

  Component {
    id: mediaCastPageComponent
    MediaCastPage {}
  }
}
