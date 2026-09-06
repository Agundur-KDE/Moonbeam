import QtQuick
import org.kde.kirigami as Kirigami
import org.agundur.moonbeam

Kirigami.ApplicationWindow {
    id: root
    title: "Moonbeam"

    width: Kirigami.Units.gridUnit * 27
    height: Kirigami.Units.gridUnit * 36
    minimumWidth: Kirigami.Units.gridUnit * 20
    minimumHeight: Kirigami.Units.gridUnit * 24

    pageStack.initialPage: StatusPage {}

    // App-wide actions (About, Quit) live here, not on a specific page -
    // the usual Kirigami convention, and the compact isMenu presentation
    // fits this app's single-purpose, no-hamburger-drawer-needed shape.
    globalDrawer: Kirigami.GlobalDrawer {
        isMenu: true

        actions: [
            Kirigami.Action {
                text: i18n("About Moonbeam")
                icon.name: "help-about"
                // pageStack.layers is a QQC2.StackView whose initialItem
                // is the normal page view itself - so depth is 1 with no
                // layer open, not 0. Guarding on the wrong baseline here
                // meant this condition was never true, so nothing ever
                // happened on click - completely independent of isMenu
                // true/false, which was a dead end chased before this was
                // found. Guards against the actual bug this depth check
                // was meant to fix: pageStack.layers.push() doesn't
                // prevent pushing onto itself while already showing a
                // layer, so clicking "About" more than once used to stack
                // another copy on top each time (the back arrow then had
                // to be clicked once per stacked copy instead of
                // returning to Main immediately - "browser history"
                // behavior).
                onTriggered: {
                    if (root.pageStack.layers.depth === 1) {
                        root.pageStack.layers.push(Qt.resolvedUrl("pages/AboutPage.qml"));
                    }
                }
            },
            Kirigami.Action {
                text: i18n("Quit")
                icon.name: "application-exit"
                shortcut: StandardKey.Quit
                onTriggered: Qt.quit()
            }
        ]
    }

    // pageStack.layers (not pageStack itself): About is a modal-like
    // standalone page, not a "detail" of Status. Pushing it onto the
    // regular column stack lets Kirigami's PageRow show it side-by-side
    // with the current page once the window is wide enough (its default
    // "wide screen" master-detail behavior), halving that page's available
    // width and clipping any translation longer than English.
    // layers.push() always covers the full window and gets a proper,
    // reliable back button - the standard Kirigami idiom for an About page.
    //
    // Pushed by URL, not a static instance or Component id: PageRow's
    // goBack() only decrements currentIndex, it doesn't actually pop the
    // page out of the stack - so a static, reused page instance would
    // still be "already in the row" the second time this fires, and
    // push() would silently no-op instead of reopening it. Pushing by URL
    // (like PairingPage.qml already does on the main pageStack) creates a
    // fresh instance every time.
}
