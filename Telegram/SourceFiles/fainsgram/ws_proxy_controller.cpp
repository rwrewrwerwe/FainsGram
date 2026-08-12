/*
 * FainsGram — TG WS Proxy Controller (Implementation)
 * Copyright (c) 2024 FainsGram. MIT License.
 */

#include "ws_proxy_controller.h"
#include "fainsgram_controller.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QStandardPaths>
#include <QtCore/QRandomGenerator>
#include <QtCore/QTimer>
#include <QtCore/QByteArray>

#include "core/application.h"
#include "core/core_settings.h"
#include "core/core_settings_proxy.h"
#include "mtproto/mtproto_proxy_data.h"

#include <algorithm>
#include <cstdint>

namespace FainsGram {

WsProxyController::WsProxyController(QObject* parent) : QObject(parent) {
    _process = new QProcess(this);
    connect(_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int code, QProcess::ExitStatus) {
        _status = QString("Stopped (exit %1)").arg(code);
        Q_EMIT statusChanged(_status);
    });
    connect(_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
        _status = QString("Launch error: %1").arg(_process->errorString());
        Q_EMIT statusChanged(_status);
    });
}

WsProxyController::~WsProxyController() {
    stopProxy();
}

// ── Path resolution ─────────────────────────────────────────────────────────────

QString WsProxyController::wsproxyDir() const {
    // Locate the vendored tg-ws-proxy source. Search upward from the
    // executable so it works from both the installed bundle and a dev build.
    const QString start = QCoreApplication::applicationDirPath();
    const QStringList fragments = {
#ifdef Q_OS_MAC
        "/../Resources/fainsgram/wsproxy/tg-ws-proxy",
        "/../../Resources/fainsgram/wsproxy/tg-ws-proxy",
#else
        "/fainsgram/wsproxy/tg-ws-proxy",
#endif
        "/Telegram/SourceFiles/fainsgram/wsproxy/tg-ws-proxy",
        "/SourceFiles/fainsgram/wsproxy/tg-ws-proxy",
    };
    QDir d(start);
    for (int depth = 0; depth < 10 && !d.isRoot(); ++depth) {
        const QString here = d.absolutePath();
        for (const QString& f : fragments) {
            const QString cand = QDir(here + f).absolutePath();
            if (QFileInfo::exists(cand + "/proxy/tg_ws_proxy.py")) return cand;
        }
        if (!d.cdUp()) break;
    }
#ifdef Q_OS_MAC
    return QDir(start + "/../Resources/fainsgram/wsproxy/tg-ws-proxy").absolutePath();
#else
    return QDir(start + "/fainsgram/wsproxy/tg-ws-proxy").absolutePath();
#endif
}

QString WsProxyController::venvDir() const {
    // The venv must live in a writable location (not inside the read-only bundle).
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + "/fainsgram/wsproxy/venv";
}

QString WsProxyController::resolvePython() const {
    const QString venv = venvDir();
#ifdef Q_OS_WIN
    const QString py = venv + "/Scripts/python.exe";
#else
    const QString py = venv + "/bin/python3";
#endif
    if (QFileInfo::exists(py)) return py;
    return "python3";   // system fallback
}

void WsProxyController::ensureRuntime() {
    // Async: create the venv, then install deps, then actually start the proxy.
    const QString venv = venvDir();
#ifdef Q_OS_WIN
    const QString py = venv + "/Scripts/python.exe";
#else
    const QString py = venv + "/bin/python3";
#endif
    if (QFileInfo::exists(py)) {
        startProxyProcess();
        return;
    }

    _status = "Preparing Python runtime (one-time setup)…";
    Q_EMIT statusChanged(_status);

    auto* mk = new QProcess(this);
    mk->setProgram("python3");
    mk->setArguments({"-m", "venv", venv});
    connect(mk, &QProcess::finished, this, [this, py, mk](int code) {
        mk->deleteLater();
        if (code != 0) {
            _status = "Could not create Python venv (is python3 installed?)";
            Q_EMIT statusChanged(_status);
            return;
        }
        auto* pip = new QProcess(this);
        pip->setProgram(py);
        pip->setArguments({"-m", "pip", "install", "-r", wsproxyDir() + "/requirements.txt"});
        connect(pip, &QProcess::finished, this, [this, pip](int code) {
            pip->deleteLater();
            if (code != 0) {
                _status = "Failed to install proxy dependencies (pip error)";
                Q_EMIT statusChanged(_status);
                return;
            }
            startProxyProcess();
        });
        pip->start();
    });
    mk->start();
}

// ── Process lifecycle ───────────────────────────────────────────────────────────

void WsProxyController::startProxy() {
    ensureRuntime();   // async; spawns startProxyProcess() once the runtime is ready
}

void WsProxyController::startProxyProcess() {
    if (!_enabled) return;   // was disabled while bootstrapping
    _process->setWorkingDirectory(wsproxyDir());
    _process->setProgram(resolvePython());
    _process->setArguments({
        "-m", "proxy.tg_ws_proxy",
        "--host", "127.0.0.1",
        "--port", QString::number(_port),
        "--secret", _secret,
        "--no-cfproxy",
    });
    _status = "Starting local proxy…";
    Q_EMIT statusChanged(_status);
    _process->start();
}

void WsProxyController::stopProxy() {
    if (_process && _process->state() != QProcess::NotRunning) {
        _process->terminate();
        if (!_process->waitForFinished(5000)) {
            _process->kill();
        }
    }
    _status = "Stopped";
    Q_EMIT statusChanged(_status);
}

// ── Telegram MTProto proxy wiring ────────────────────────────────────────────────

void WsProxyController::applyTelegramProxy(bool enable) {
    auto& proxy = Core::App().settings().proxy();

    if (enable) {
        MTP::ProxyData d;
        d.type  = MTP::ProxyData::Type::Mtproto;
        d.host  = "127.0.0.1";
        d.port  = uint32_t(_port);
        d.password = "dd" + _secret;   // dd = default MTProto secret prefix

        auto& list = proxy.list();
        bool found = false;
        for (auto& p : list) {
            if (p.type == MTP::ProxyData::Type::Mtproto
                && p.host == "127.0.0.1"
                && p.port == uint32_t(_port)) {
                p = d;
                found = true;
                break;
            }
        }
        if (!found) list.push_back(d);

        proxy.setSelected(d);
        proxy.setSettings(MTP::ProxyData::Settings::Enabled);
        proxy.connectionTypeChangesNotify();
        _status = QString("Running · proxy active on 127.0.0.1:%1").arg(_port);
        Q_EMIT statusChanged(_status);
    } else {
        auto& list = proxy.list();
        list.erase(std::remove_if(list.begin(), list.end(),
            [](const MTP::ProxyData& p) {
                return p.type == MTP::ProxyData::Type::Mtproto
                    && p.host == "127.0.0.1";
            }), list.end());
        proxy.setSettings(MTP::ProxyData::Settings::System);
        proxy.connectionTypeChangesNotify();
    }
}

// ── Public API ──────────────────────────────────────────────────────────────────

void WsProxyController::setEnabled(bool on) {
    if (_enabled == on) return;
    _enabled = on;
    if (on) {
        startProxy();
        applyTelegramProxy(true);
    } else {
        stopProxy();
        applyTelegramProxy(false);
    }
    saveSettings();
    Q_EMIT enabledChanged(on);
}

void WsProxyController::setPort(int port) {
    if (port < 1 || port > 65535) return;
    _port = port;
    saveSettings();
}

void WsProxyController::setSecret(const QString& secret) {
    const QByteArray hex = QByteArray::fromHex(secret.trimmed().toUtf8());
    if (hex.length() != 16) return;   // 32 hex chars expected
    _secret = secret.trimmed();
    saveSettings();
}

bool WsProxyController::isRunning() const {
    return _process && _process->state() == QProcess::Running;
}

// ── Persistence ──────────────────────────────────────────────────────────────────

void WsProxyController::loadSettings() {
    auto& s = FG().settings();
    _enabled = s.value("WsProxy/enabled", false).toBool();
    _port    = s.value("WsProxy/port", 1443).toInt();
    _secret  = s.value("WsProxy/secret", "").toString();
    if (_secret.length() != 32) {
        QByteArray raw(16, 0);
        auto* rnd = QRandomGenerator::global();
        for (int i = 0; i < 16; ++i) raw[i] = uint8_t(rnd->generate() & 0xFF);
        _secret = QString::fromUtf8(raw.toHex());
        s.setValue("WsProxy/secret", _secret);
        s.sync();
    }
}

void WsProxyController::saveSettings() {
    auto& s = FG().settings();
    s.setValue("WsProxy/enabled", _enabled);
    s.setValue("WsProxy/port", _port);
    s.setValue("WsProxy/secret", _secret);
    s.sync();
}

} // namespace FainsGram
