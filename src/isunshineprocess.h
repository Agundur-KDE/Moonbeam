// SPDX-FileCopyrightText: 2026 Agundur <info@agundur.de>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>

/**
 * Seam around the long-running Sunshine process SunshineController starts
 * and stops, so tests can simulate crashes, timeouts, and "not installed"
 * without spawning a real process (audit.txt R-01).
 */
class ISunshineProcess : public QObject
{
    Q_OBJECT

public:
    explicit ISunshineProcess(QObject *parent = nullptr)
        : QObject(parent)
    {
    }
    ~ISunshineProcess() override = default;

    virtual void start(const QString &program, const QStringList &arguments) = 0;
    virtual bool waitForStarted(int timeoutMs) = 0;
    virtual void terminate() = 0;
    virtual void kill() = 0;
    virtual bool waitForFinished(int timeoutMs) = 0;
    virtual QProcess::ProcessState state() const = 0;

Q_SIGNALS:
    void started();
    void finished();
    void errorOccurred();
};

/** Real implementation: forwards everything to an owned QProcess. */
class QtSunshineProcess : public ISunshineProcess
{
    Q_OBJECT

public:
    explicit QtSunshineProcess(QObject *parent = nullptr);

    void start(const QString &program, const QStringList &arguments) override;
    bool waitForStarted(int timeoutMs) override;
    void terminate() override;
    void kill() override;
    bool waitForFinished(int timeoutMs) override;
    QProcess::ProcessState state() const override;

private:
    QProcess m_process;
};
