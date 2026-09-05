#include "sunshinecontroller.h"

#include <KAboutData>
#include <KLocalizedString>

#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    KLocalizedString::setApplicationDomain("moonbeam");

    KAboutData aboutData(QStringLiteral("moonbeam"),
                          i18n("Moonbeam"),
                          QStringLiteral("0.1.0"),
                          i18n("Share your desktop over the network via Sunshine/Moonlight"),
                          KAboutLicense::GPL_V3,
                          i18n("(c) 2026 Agundur"));
    KAboutData::setApplicationData(aboutData);
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("org.agundur.moonbeam")));

    qmlRegisterType<SunshineController>("org.agundur.moonbeam", 1, 0, "SunshineController");

    QQmlApplicationEngine engine;
    engine.loadFromModule("org.agundur.moonbeam", "Main");

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
