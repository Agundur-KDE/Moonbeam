#include <KAboutData>
#include <KLocalizedString>

#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>

#include <cstdio>

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

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine, &QQmlApplicationEngine::warnings, &engine, [](const QList<QQmlError> &warnings) {
            for (const auto &warning : warnings) {
                std::fprintf(stderr, "QML WARNING: %s\n", qPrintable(warning.toString()));
            }
        });
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &engine, [] {
            std::fprintf(stderr, "QML object creation failed\n");
        });

    engine.loadFromModule("org.agundur.moonbeam", "Main");

    if (engine.rootObjects().isEmpty()) {
        std::fprintf(stderr, "moonbeam: no root objects after loadFromModule\n");
        return -1;
    }

    return app.exec();
}
