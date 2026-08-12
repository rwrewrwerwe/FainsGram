/*
 * FainsGram — TG WS Proxy Controller
 * ====================================
 * Owns and controls the bundled tg-ws-proxy (local MTProto proxy).
 *  - Launches the proxy headlessly (no system tray) as a child process.
 *  - Auto-configures Telegram's own MTProto proxy to 127.0.0.1:<port>
 *    with the shared secret, so Telegram connects automatically.
 *  - Persists enabled / port / secret in the FainsGram QSettings.
 *
 * Copyright (c) 2024 FainsGram. MIT License.
 */

#pragma once

#include <QtCore/QObject>
#include <QtCore/QProcess>
#include <QtCore/QString>

namespace FainsGram {

class WsProxyController final : public QObject {
    Q_OBJECT
public:
    explicit WsProxyController(QObject* parent = nullptr);
    ~WsProxyController() override;

    void loadSettings();
    void saveSettings();

    bool    isEnabled()  const { return _enabled; }
    bool    isRunning()  const;
    QString statusText() const { return _status; }
    int     port()       const { return _port; }
    QString secret()     const { return _secret; }

    void setEnabled(bool on);
    void setPort(int port);
    void setSecret(const QString& secret);

Q_SIGNALS:
    void enabledChanged(bool on);
    void statusChanged(const QString& status);

private:
    void startProxy();
    void startProxyProcess();
    void stopProxy();
    void applyTelegramProxy(bool enable);
    QString resolvePython() const;
    QString wsproxyDir() const;
    QString venvDir() const;
    void ensureRuntime();

    bool    _enabled = false;
    int     _port    = 1443;
    QString _secret;
    QString _status  = "Stopped";
    QProcess* _process = nullptr;
};

} // namespace FainsGram
