#include "isunshinenetworkprobe.h"

#include <QSslSocket>
#include <QTcpSocket>

bool QtSunshineNetworkProbe::isPortOpen(quint16 port, int timeoutMs) const
{
    QTcpSocket socket;
    socket.connectToHost(QStringLiteral("127.0.0.1"), port);
    const bool connected = socket.waitForConnected(timeoutMs);
    socket.abort();
    return connected;
}

QSslCertificate QtSunshineNetworkProbe::peerCertificateAt(quint16 port, int timeoutMs) const
{
    QSslSocket socket;
    socket.setPeerVerifyMode(QSslSocket::VerifyNone);
    socket.connectToHostEncrypted(QStringLiteral("127.0.0.1"), port);
    if (!socket.waitForEncrypted(timeoutMs)) {
        return QSslCertificate();
    }

    const QSslCertificate cert = socket.peerCertificate();
    socket.abort();
    return cert;
}
