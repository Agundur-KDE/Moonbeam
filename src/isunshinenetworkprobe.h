#pragma once

#include <QSslCertificate>
#include <QString>

/**
 * Seam around the raw socket I/O SunshineController uses to check whether
 * something is listening on Sunshine's web UI port and, if so, what
 * certificate it presents - so tests can simulate an unreachable port, a
 * different service entirely, a slow/hanging handshake, or a spoofed
 * certificate without opening a real socket (audit.txt R-01).
 */
class ISunshineNetworkProbe
{
public:
    virtual ~ISunshineNetworkProbe() = default;

    virtual bool isPortOpen(quint16 port, int timeoutMs) const = 0;

    // The certificate presented by a TLS handshake against 127.0.0.1:port,
    // or a null QSslCertificate if unreachable or the handshake failed/
    // timed out.
    virtual QSslCertificate peerCertificateAt(quint16 port, int timeoutMs) const = 0;
};

/** Real implementation: QTcpSocket for the port check, QSslSocket for the handshake. */
class QtSunshineNetworkProbe : public ISunshineNetworkProbe
{
public:
    bool isPortOpen(quint16 port, int timeoutMs) const override;
    QSslCertificate peerCertificateAt(quint16 port, int timeoutMs) const override;
};
