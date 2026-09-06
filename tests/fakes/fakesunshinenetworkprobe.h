#pragma once

#include "isunshinenetworkprobe.h"

/**
 * Test double for ISunshineNetworkProbe: no real socket is ever opened.
 * Tests configure whether the port answers and what certificate (if any)
 * a TLS handshake against it would present - including a spoofed
 * certificate with the real CN but a different key, to reproduce the
 * exact S-01 exploit shape at the controller level.
 */
class FakeSunshineNetworkProbe : public ISunshineNetworkProbe
{
public:
    bool portOpen = false;
    QSslCertificate certificateAtPort;

    bool isPortOpen(quint16, int) const override
    {
        return portOpen;
    }

    QSslCertificate peerCertificateAt(quint16, int) const override
    {
        return certificateAtPort;
    }
};
