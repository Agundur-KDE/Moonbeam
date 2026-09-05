import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.ScrollablePage {
    title: qsTr("Cast Media")

    ColumnLayout {
        width: parent.width
        spacing: Kirigami.Units.largeSpacing

        Kirigami.PlaceholderMessage {
            Layout.fillWidth: true
            icon.name: "media-playback-start"
            text: qsTr("Media casting lives here")
            explanation: qsTr("Sketch stage: this tab will reuse KCast's existing drag & drop cast flow.")
        }
    }
}
