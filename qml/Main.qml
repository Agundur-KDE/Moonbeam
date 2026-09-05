import QtQuick
import org.kde.kirigami as Kirigami
import org.agundur.moonbeam

Kirigami.ApplicationWindow {
    id: root
    title: qsTr("Moonbeam")

    width: 480
    height: 640

    pageStack.initialPage: StatusPage {}
}
