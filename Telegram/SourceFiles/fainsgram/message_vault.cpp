/*
 * FainsGram — Message Vault (Implementation)
 * Copyright (c) 2024 FainsGram. MIT License.
 */

#include "message_vault.h"
#include "fainsgram_controller.h"

#include <QtCore/QDir>
#include <QtCore/QStandardPaths>
#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <openssl/evp.h>
#include <openssl/rand.h>

namespace FainsGram {

static const char* kDbConn = "FainsGramVault";

MessageVault::MessageVault(QObject* parent) : QObject(parent) {}
MessageVault::~MessageVault() { if (_db.isOpen()) _db.close(); }

void MessageVault::initialize() {
    if (_initialized) return;
    const QString dir = QStandardPaths::writableLocation(
        QStandardPaths::AppDataLocation) + "/FainsGram";
    QDir().mkpath(dir);
    _db = QSqlDatabase::addDatabase("QSQLITE", kDbConn);
    _db.setDatabaseName(dir + "/vault.db");
    if (initDb()) _initialized = true;
}

bool MessageVault::initDb() {
    if (!_db.open()) return false;
    QSqlQuery q(_db);
    q.exec("PRAGMA journal_mode=WAL;");
    q.exec("PRAGMA synchronous=NORMAL;");
    return q.exec(R"SQL(
        CREATE TABLE IF NOT EXISTS msgs (
            id        INTEGER PRIMARY KEY AUTOINCREMENT,
            msg_id    INTEGER, peer_id   INTEGER, peer_name TEXT,
            from_id   INTEGER, from_name TEXT,
            txt       BLOB,    media     TEXT,
            ts        INTEGER, saved     INTEGER, type INTEGER
        ))SQL");
}

void MessageVault::capture(
        qint64 msgId, qint64 peerId, const QString& peerName,
        qint64 fromId, const QString& fromName,
        const QString& text, const QString& media,
        QDateTime ts, VaultMessageType type) {

    if (!_initialized) return;
    QSqlQuery q(_db);
    q.prepare("INSERT INTO msgs VALUES(NULL,?,?,?,?,?,?,?,?,?,?)");
    q.addBindValue(msgId); q.addBindValue(peerId); q.addBindValue(peerName);
    q.addBindValue(fromId); q.addBindValue(fromName);
    q.addBindValue(_encrypted ? encrypt(text) : text.toUtf8());
    q.addBindValue(media);
    q.addBindValue(ts.toMSecsSinceEpoch());
    q.addBindValue(QDateTime::currentMSecsSinceEpoch());
    q.addBindValue((int)type);
    if (!q.exec()) return;

    VaultMessage m;
    m.id=q.lastInsertId().toLongLong(); m.msgId=msgId;
    m.peerId=peerId; m.peerName=peerName;
    m.fromId=fromId; m.fromName=fromName;
    m.text=text; m.mediaPath=media;
    m.timestamp=ts; m.savedAt=QDateTime::currentDateTime(); m.type=type;
    Q_EMIT captured(m);
}

VaultMessage MessageVault::fromQuery(QSqlQuery& q) const {
    VaultMessage m;
    m.id       = q.value("id").toLongLong();
    m.msgId    = q.value("msg_id").toLongLong();
    m.peerId   = q.value("peer_id").toLongLong();
    m.peerName = q.value("peer_name").toString();
    m.fromId   = q.value("from_id").toLongLong();
    m.fromName = q.value("from_name").toString();
    QByteArray raw = q.value("txt").toByteArray();
    m.text     = _encrypted ? decrypt(raw) : QString::fromUtf8(raw);
    m.mediaPath= q.value("media").toString();
    m.timestamp= QDateTime::fromMSecsSinceEpoch(q.value("ts").toLongLong());
    m.savedAt  = QDateTime::fromMSecsSinceEpoch(q.value("saved").toLongLong());
    m.type     = (VaultMessageType)q.value("type").toInt();
    return m;
}

std::vector<VaultMessage> MessageVault::all() const {
    std::vector<VaultMessage> r;
    if (!_initialized) return r;
    QSqlQuery q("SELECT * FROM msgs ORDER BY ts DESC", _db);
    while (q.next()) r.push_back(fromQuery(q));
    return r;
}

std::vector<VaultMessage> MessageVault::forPeer(qint64 peerId) const {
    std::vector<VaultMessage> r;
    if (!_initialized) return r;
    QSqlQuery q(_db);
    q.prepare("SELECT * FROM msgs WHERE peer_id=? ORDER BY ts DESC");
    q.addBindValue(peerId); q.exec();
    while (q.next()) r.push_back(fromQuery(q));
    return r;
}

std::vector<VaultMessage> MessageVault::search(const QString& query) const {
    std::vector<VaultMessage> r;
    if (!_initialized || _encrypted) return r;
    QSqlQuery q(_db);
    q.prepare("SELECT * FROM msgs WHERE CAST(txt AS TEXT) LIKE ?");
    q.addBindValue("%" + query + "%"); q.exec();
    while (q.next()) r.push_back(fromQuery(q));
    return r;
}

void MessageVault::remove(qint64 id) {
    if (!_initialized) return;
    QSqlQuery q(_db);
    q.prepare("DELETE FROM msgs WHERE id=?");
    q.addBindValue(id); q.exec();
}

void MessageVault::clear() {
    if (!_initialized) return;
    QSqlQuery("DELETE FROM msgs", _db).exec();
}

bool MessageVault::exportToPath(const QString& dir) const {
    QDir().mkpath(dir);
    QJsonArray arr;
    for (const auto& m : all()) {
        QJsonObject o;
        o["peer"]=m.peerName; o["from"]=m.fromName;
        o["text"]=m.text; o["time"]=m.timestamp.toString(Qt::ISODate);
        o["type"]=(int)m.type;
        arr.append(o);
    }
    QFile f(dir + "/vault.json");
    if (!f.open(QIODevice::WriteOnly)) return false;
    f.write(QJsonDocument(arr).toJson());
    return true;
}

bool MessageVault::syncToiCloud() const {
    return exportToPath(
        QDir::homePath() +
        "/Library/Mobile Documents/com~apple~CloudDocs/FainsGram");
}

void MessageVault::exportToiCloudIfEnabled() {
    auto& s = FG().settings();
    if (s.value("Vault/iCloudSync", false).toBool())
        syncToiCloud();
}

// ── AES-256-CBC encryption ───────────────────────────────────────────────────

void MessageVault::setKey(const QByteArray& key32) {
    _key = key32.left(32).leftJustified(32, '\0');
    _encrypted = !_key.isEmpty();
}

QByteArray MessageVault::encrypt(const QString& t) const {
    if (!_encrypted) return t.toUtf8();
    QByteArray pt = t.toUtf8();
    QByteArray iv(16, 0); RAND_bytes((uchar*)iv.data(), 16);
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr,
        (uchar*)_key.data(), (uchar*)iv.data());
    QByteArray ct(pt.size()+16, 0); int l1=0,l2=0;
    EVP_EncryptUpdate(ctx,(uchar*)ct.data(),&l1,(uchar*)pt.data(),pt.size());
    EVP_EncryptFinal_ex(ctx,(uchar*)ct.data()+l1,&l2);
    EVP_CIPHER_CTX_free(ctx); ct.resize(l1+l2);
    return iv + ct;
}

QString MessageVault::decrypt(const QByteArray& b) const {
    if (!_encrypted || b.size() < 17) return QString::fromUtf8(b);
    QByteArray iv=b.left(16), ct=b.mid(16);
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr,
        (uchar*)_key.data(), (uchar*)iv.data());
    QByteArray pt(ct.size(),0); int l1=0,l2=0;
    EVP_DecryptUpdate(ctx,(uchar*)pt.data(),&l1,(uchar*)ct.data(),ct.size());
    EVP_DecryptFinal_ex(ctx,(uchar*)pt.data()+l1,&l2);
    EVP_CIPHER_CTX_free(ctx); pt.resize(l1+l2);
    return QString::fromUtf8(pt);
}

} // namespace FainsGram
