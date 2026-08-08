/*
 * FainsGram — Vault Helper (inline header)
 * =========================================
 * Самодостаточный helper для захвата сообщений в SQLite vault.
 * Намеренно header-only чтобы можно вставить в любой .cpp без изменения CMakeLists.
 *
 * Использование:
 *   #include "fainsgram/vault_helper.h"
 *   FainsVault::capture(item, FainsVault::Type::Deleted);
 *
 * Copyright (c) 2024 FainsGram. MIT License.
 */

#pragma once

#include <QtCore/QSettings>
#include <QtCore/QStandardPaths>
#include <QtCore/QDir>
#include <QtCore/QString>
#include <QtCore/QDateTime>
#include <QtCore/QMutex>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtCore/QDebug>

// Forward declare TDesktop type — включается после tdesktop headers
// class HistoryItem;

namespace FainsVault {

enum class Type { Deleted = 0, Disappearing = 1, Edited = 2 };

// ─────────────────────────────────────────────────────────────────────────────
// Внутренняя БД — открывается один раз, thread-safe через мьютекс
// ─────────────────────────────────────────────────────────────────────────────

namespace detail {

inline QMutex& mutex() {
    static QMutex m;
    return m;
}

inline bool& dbReady() {
    static bool v = false;
    return v;
}

inline bool ensureDb() {
    if (dbReady()) return true;

    const QString dir = QStandardPaths::writableLocation(
        QStandardPaths::AppDataLocation) + "/FainsGram";
    QDir().mkpath(dir);

    auto db = QSqlDatabase::addDatabase("QSQLITE", "FainsVaultDB");
    db.setDatabaseName(dir + "/vault.db");
    if (!db.open()) {
        qWarning() << "[FainsGram] Vault: cannot open DB:" << db.lastError().text();
        return false;
    }

    QSqlQuery q(db);
    q.exec("PRAGMA journal_mode=WAL;");
    q.exec("PRAGMA synchronous=NORMAL;");
    const bool ok = q.exec(R"(
        CREATE TABLE IF NOT EXISTS msgs (
            id        INTEGER PRIMARY KEY AUTOINCREMENT,
            msg_id    INTEGER,
            peer_id   INTEGER,
            peer_name TEXT,
            from_id   INTEGER,
            from_name TEXT,
            txt       TEXT,
            media     TEXT,
            ts        INTEGER,
            saved     INTEGER,
            type      INTEGER
        )
    )");
    if (!ok) {
        qWarning() << "[FainsGram] Vault: table create failed:" << q.lastError().text();
        return false;
    }
    q.exec("CREATE INDEX IF NOT EXISTS idx_peer ON msgs(peer_id);");

    dbReady() = true;
    qDebug() << "[FainsGram] Vault: opened at" << (dir + "/vault.db");
    return true;
}

} // namespace detail

// ─────────────────────────────────────────────────────────────────────────────
// Проверка: включён ли vault в настройках
// ─────────────────────────────────────────────────────────────────────────────

inline bool isEnabled() {
    static const bool v = [](){
        const QString cfg = QStandardPaths::writableLocation(
            QStandardPaths::AppConfigLocation) + "/FainsGram/fainsgram.ini";
        QSettings s(cfg, QSettings::IniFormat);
        // Vault включён по умолчанию
        return s.value("Vault/enabled", true).toBool();
    }();
    return v;
}

// ─────────────────────────────────────────────────────────────────────────────
// Основная функция захвата — вызывается ДО удаления/истечения сообщения
// ─────────────────────────────────────────────────────────────────────────────

inline void saveRaw(
        qint64 msgId,
        qint64 peerId, const QString& peerName,
        qint64 fromId, const QString& fromName,
        const QString& text, const QString& mediaPath,
        qint64 timestampMs, Type type) {

    if (!isEnabled()) return;

    QMutexLocker lock(&detail::mutex());
    if (!detail::ensureDb()) return;

    auto db = QSqlDatabase::database("FainsVaultDB");
    QSqlQuery q(db);
    q.prepare(R"(
        INSERT INTO msgs
            (msg_id, peer_id, peer_name, from_id, from_name, txt, media, ts, saved, type)
        VALUES (?,?,?,?,?,?,?,?,?,?)
    )");
    q.addBindValue(msgId);
    q.addBindValue(peerId);
    q.addBindValue(peerName);
    q.addBindValue(fromId);
    q.addBindValue(fromName);
    q.addBindValue(text);
    q.addBindValue(mediaPath);
    q.addBindValue(timestampMs);
    q.addBindValue(QDateTime::currentMSecsSinceEpoch());
    q.addBindValue(static_cast<int>(type));

    if (!q.exec()) {
        qWarning() << "[FainsGram] Vault: insert failed:" << q.lastError().text();
    } else {
        qDebug() << "[FainsGram] Vault: saved msg" << msgId
                 << "from peer" << peerName
                 << "type" << static_cast<int>(type);
    }
}

} // namespace FainsVault
