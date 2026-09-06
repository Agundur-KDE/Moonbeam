#pragma once

#include <QByteArray>
#include <QSslCertificate>
#include <QString>

/**
 * Resolves which certificate Moonbeam should trust as "the local Sunshine
 * instance", and computes/compares its identity.
 *
 * Replaces the old "does the peer certificate's CN say Sunshine Gamestream
 * Host" check (S-01 in audit.txt): that CN is a public, hardcoded string -
 * any local process can put it in a self-signed certificate of its own.
 * Instead this pins the exact certificate Sunshine itself is configured to
 * serve, read from sunshine.conf (or Sunshine's documented default path if
 * unconfigured), and compares by SHA-256 fingerprint of the DER encoding -
 * not by any field inside the certificate that an impostor could also set.
 */
namespace SunshineIdentity
{
/**
 * Path to Sunshine's server certificate. Honors an explicit `cert = ...`
 * line in sunshine.conf (see Sunshine's config.cpp), resolving a relative
 * path against configDir the same way Sunshine resolves it against its own
 * config directory. Falls back to Sunshine's default
 * "<configDir>/credentials/cacert.pem" when no `cert` line is present.
 */
QString certificatePath(const QString &configDir, const QString &configContents);

/**
 * SHA-256 fingerprint of the certificate at certPath, or an empty
 * QByteArray if the file is missing, unreadable, or not a valid
 * certificate. Callers must treat an empty result as "cannot verify this
 * is Sunshine" (fail closed) - never as a wildcard match.
 */
QByteArray certificateFingerprint(const QString &certPath);

/**
 * Loads the certificate at certPath, or a null QSslCertificate if the file
 * is missing, unreadable, or not a valid certificate. Used to install the
 * pinned certificate itself as the *sole* trusted CA for a connection (see
 * SunshineController::pair(), audit.txt S-01) - unlike a fingerprint check
 * that only runs inside an sslErrors handler, this makes an unpinned
 * certificate fail chain validation structurally, even one that some
 * other, broader trust store would otherwise accept without error.
 */
QSslCertificate loadCertificate(const QString &certPath);

/**
 * True iff certificate's SHA-256 digest exactly matches pinnedFingerprint.
 * An empty pinnedFingerprint (identity could not be resolved) always
 * returns false - "cannot verify" must never be treated as "matches
 * anything".
 */
bool matchesPinnedFingerprint(const QSslCertificate &certificate, const QByteArray &pinnedFingerprint);
}
