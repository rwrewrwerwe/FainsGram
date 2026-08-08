/*
 * FainsGram — Message Vault
 * ==========================
 * Captures messages BEFORE deletion/expiry into an encrypted SQLite database.
 *
 * Captured events:
 *   1. deleteMessages / deleteHistory  → "Deleted" vault entry
 *   2. TTL / self-destruct countdown  → "Disappearing" vault entry
 *   3. Message edits (old version)    → "Edited" vault entry
 *
 * Storage: <AppData>/FainsGram/vault.db  (AES-256-CBC encrypted if passcode set)
 * macOS sync: ~/Library/Mobile Documents/com~apple~CloudDocs/FainsGram/
 *
 * Copyright (c) 2024 FainsGram. MIT License.
 */

#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QDateTime>
#include <QtSql/QSqlDatabase>
#include <vector>
#include <optional>

namespace FainsGram {

enum class VaultMessageType { Deleted = 0, Disappearing = 1, Edited = 2 };

struct VaultMessage {
    qint64          id        = 0;
    qint64          msgId     = 0;
    qint64          peerId    = 0;
    QString         peerName;
    qint64          fromId    = 0;
    QString         fromName;
    QString         text;
    QString         mediaPath;
    QDateTime       timestamp;
    QDateTime       savedAt;
    VaultMessageType type     = VaultMessageType::Deleted;
};

class MessageVault final : public QObject {
    Q_OBJECT
public:
    explicit MessageVault(QObject* parent = nullptr);
    ~MessageVault() override;

    void initialize();

    // Hook called before a message is removed / expires
    void capture(qint64 msgId,
                 qint64 peerId, const QString& peerName,
                 qint64 fromId, const QString& fromName,
                 const QString& text, const QString& mediaLocalPath,
                 QDateTime timestamp, VaultMessageType type);

    std::vector<VaultMessage> all() const;
    std::vector<VaultMessage> forPeer(qint64 peerId) const;
    std::vector<VaultMessage> search(const QString& q) const;

    void remove(qint64 vaultId);
    void clear();

    // macOS iCloud export
    bool exportToPath(const QString& dir) const;
    bool syncToiCloud() const;
    void exportToiCloudIfEnabled();

    // Encryption
    void setKey(const QByteArray& key32);
    bool encrypted() const { return _encrypted; }

Q_SIGNALS:
    void captured(const VaultMessage& msg);

private:
    bool        initDb();
    QByteArray  encrypt(const QString& t) const;
    QString     decrypt(const QByteArray& b) const;
    VaultMessage fromQuery(class QSqlQuery& q) const;

    QSqlDatabase _db;
    QByteArray   _key;
    bool         _encrypted  = false;
    bool         _initialized = false;
};

} // namespace FainsGram
