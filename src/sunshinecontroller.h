#pragma once

#include <QObject>
#include <QProcess>
#include <qqmlintegration.h>

/**
 * Thin controller around a Sunshine process.
 *
 * Sketch stage: only process lifecycle + placeholder status. Pairing/PIN
 * handling, config generation, and firewall setup are not implemented yet.
 */
class SunshineController : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY runningChanged)

public:
    explicit SunshineController(QObject *parent = nullptr);

    bool running() const;
    QString statusText() const;

public Q_SLOTS:
    void start();
    void stop();

Q_SIGNALS:
    void runningChanged();

private:
    QProcess m_process;
};
