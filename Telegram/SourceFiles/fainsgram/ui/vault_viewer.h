/*
 * FainsGram — Vault Viewer Widget
 * =================================
 * Окно просмотра сохранённых удалённых/исчезающих сообщений.
 * Открывается из панели настроек FainsGram → вкладка "Vault".
 *
 * Показывает:
 *  - Все сохранённые сообщения (удалённые / исчезающие)
 *  - Имя собеседника, отправителя, дату, текст, тип
 *  - Иконку 🗑 для удалённых, ⏱ для исчезающих
 *  - Поиск по тексту
 *  - Кнопки: удалить из vault, экспортировать
 *
 * Copyright (c) 2024 FainsGram. MIT License.
 */

#pragma once

#include <QtWidgets/QWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QListWidgetItem>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QTextBrowser>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QFileDialog>
#include <QtCore/QDateTime>
#include <QtCore/QMutex>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtCore/QStandardPaths>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QFile>
#include <QtGui/QColor>
#include <QtGui/QPalette>

namespace FainsGram {

struct VaultEntry {
    qint64  id        = 0;
    qint64  msgId     = 0;
    qint64  peerId    = 0;
    QString peerName;
    QString fromName;
    QString text;
    QString mediaPath;
    QDateTime timestamp;
    QDateTime savedAt;
    int     type      = 0;  // 0=Deleted, 1=Disappearing
};

// ── Vault data access ─────────────────────────────────────────────────────────
inline QVector<VaultEntry> loadVaultEntries(const QString& filter = {}) {
    QVector<VaultEntry> result;
    auto db = QSqlDatabase::database("FainsVaultDB");
    if (!db.isOpen()) return result;

    QSqlQuery q(db);
    if (filter.isEmpty()) {
        q.prepare("SELECT * FROM msgs ORDER BY ts DESC LIMIT 500");
    } else {
        q.prepare("SELECT * FROM msgs WHERE txt LIKE ? ORDER BY ts DESC LIMIT 500");
        q.addBindValue("%" + filter + "%");
    }
    if (!q.exec()) return result;

    while (q.next()) {
        VaultEntry e;
        e.id        = q.value("id").toLongLong();
        e.msgId     = q.value("msg_id").toLongLong();
        e.peerId    = q.value("peer_id").toLongLong();
        e.peerName  = q.value("peer_name").toString();
        e.fromName  = q.value("from_name").toString();
        e.text      = q.value("txt").toString();
        e.mediaPath = q.value("media").toString();
        e.timestamp = QDateTime::fromMSecsSinceEpoch(q.value("ts").toLongLong());
        e.savedAt   = QDateTime::fromMSecsSinceEpoch(q.value("saved").toLongLong());
        e.type      = q.value("type").toInt();
        result.append(e);
    }
    return result;
}

// ── Main viewer widget ────────────────────────────────────────────────────────
class VaultViewerWidget final : public QWidget {
    Q_OBJECT

public:
    explicit VaultViewerWidget(QWidget* parent = nullptr)
        : QWidget(parent) {
        setWindowTitle("⚡ FainsGram — Message Vault");
        setMinimumSize(700, 500);
        resize(820, 600);
        setupUi();
        applyStyle();
        refresh();
    }

private:
    void setupUi() {
        auto* mainLay = new QVBoxLayout(this);
        mainLay->setContentsMargins(0, 0, 0, 0);
        mainLay->setSpacing(0);

        // ── Top bar ───────────────────────────────────────────────────────────
        auto* topBar = new QWidget;
        topBar->setObjectName("topBar");
        topBar->setFixedHeight(56);
        auto* topLay = new QHBoxLayout(topBar);
        topLay->setContentsMargins(16, 8, 16, 8);
        topLay->setSpacing(12);

        auto* titleLbl = new QLabel("🗄 Message Vault");
        titleLbl->setStyleSheet("font-size:16px;font-weight:bold;color:#ff6b35;");

        _search = new QLineEdit;
        _search->setPlaceholderText("🔍  Search messages...");
        _search->setFixedWidth(220);
        connect(_search, &QLineEdit::textChanged, this, &VaultViewerWidget::refresh);

        _typeFilter = new QComboBox;
        _typeFilter->addItem("All types");
        _typeFilter->addItem("🗑 Deleted");
        _typeFilter->addItem("⏱ Disappearing");
        connect(_typeFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &VaultViewerWidget::refresh);

        auto* refreshBtn = new QPushButton("↺ Refresh");
        refreshBtn->setObjectName("actionBtn");
        connect(refreshBtn, &QPushButton::clicked, this, &VaultViewerWidget::refresh);

        auto* exportBtn = new QPushButton("⬇ Export JSON");
        exportBtn->setObjectName("actionBtn");
        connect(exportBtn, &QPushButton::clicked, this, &VaultViewerWidget::exportJson);

        auto* clearBtn = new QPushButton("🗑 Clear All");
        clearBtn->setObjectName("dangerBtn");
        connect(clearBtn, &QPushButton::clicked, this, &VaultViewerWidget::clearVault);

        topLay->addWidget(titleLbl);
        topLay->addStretch();
        topLay->addWidget(_search);
        topLay->addWidget(_typeFilter);
        topLay->addWidget(refreshBtn);
        topLay->addWidget(exportBtn);
        topLay->addWidget(clearBtn);

        mainLay->addWidget(topBar);

        // ── Count bar ─────────────────────────────────────────────────────────
        _countLabel = new QLabel("Loading...");
        _countLabel->setContentsMargins(16, 4, 16, 4);
        _countLabel->setStyleSheet("color:#707890;font-size:12px;");
        mainLay->addWidget(_countLabel);

        // ── Splitter: list | detail ───────────────────────────────────────────
        auto* splitter = new QSplitter(Qt::Horizontal);
        splitter->setHandleWidth(1);

        // Left: message list
        _list = new QListWidget;
        _list->setObjectName("vaultList");
        _list->setMinimumWidth(280);
        connect(_list, &QListWidget::currentRowChanged,
                this, &VaultViewerWidget::showDetail);

        // Right: message detail
        auto* rightPanel = new QWidget;
        auto* rightLay = new QVBoxLayout(rightPanel);
        rightLay->setContentsMargins(0, 0, 0, 0);
        rightLay->setSpacing(0);

        _header = new QLabel("Select a message");
        _header->setContentsMargins(16, 12, 16, 8);
        _header->setStyleSheet("font-size:14px;font-weight:bold;color:#f0f2ff;");
        _header->setWordWrap(true);

        _detail = new QTextBrowser;
        _detail->setObjectName("vaultDetail");
        _detail->setOpenLinks(false);

        auto* btnRow = new QHBoxLayout;
        btnRow->setContentsMargins(12, 8, 12, 12);
        _deleteEntryBtn = new QPushButton("Remove from Vault");
        _deleteEntryBtn->setObjectName("dangerBtn");
        _deleteEntryBtn->setEnabled(false);
        connect(_deleteEntryBtn, &QPushButton::clicked,
                this, &VaultViewerWidget::deleteCurrentEntry);
        btnRow->addWidget(_deleteEntryBtn);
        btnRow->addStretch();

        rightLay->addWidget(_header);
        rightLay->addWidget(_detail, 1);
        rightLay->addLayout(btnRow);

        splitter->addWidget(_list);
        splitter->addWidget(rightPanel);
        splitter->setSizes({280, 520});

        mainLay->addWidget(splitter, 1);
    }

    void applyStyle() {
        setStyleSheet(R"(
            VaultViewerWidget { background: #10141d; }
            #topBar { background: #13172280;
                      border-bottom: 1px solid #1f2535; }
            #vaultList {
                background: #13172280;
                border: none;
                outline: none;
                color: #f0f2ff;
            }
            #vaultList::item {
                padding: 10px 14px;
                border-bottom: 1px solid #1a1f2d;
            }
            #vaultList::item:selected {
                background: rgba(255,107,53,0.18);
                color: #ff6b35;
            }
            #vaultList::item:hover { background: #1a1f2d80; }
            #vaultDetail {
                background: #0c0f18;
                color: #d0d8f0;
                border: none;
                font-size: 13px;
                padding: 16px;
            }
            QPushButton#actionBtn {
                background: #ff6b35; color: white; border: none;
                border-radius: 6px; padding: 6px 14px; font-weight: bold;
            }
            QPushButton#actionBtn:hover { background: #ff8555; }
            QPushButton#dangerBtn {
                background: rgba(255,60,60,0.15); color: #ff5555;
                border: 1px solid rgba(255,60,60,0.3);
                border-radius: 6px; padding: 6px 14px;
            }
            QPushButton#dangerBtn:hover { background: rgba(255,60,60,0.25); }
            QComboBox {
                background: #1f2535; border: 1px solid #2a3045;
                border-radius: 6px; color: #f0f2ff; padding: 4px 10px;
            }
            QLineEdit {
                background: #1f2535; border: 1px solid #2a3045;
                border-radius: 6px; color: #f0f2ff; padding: 4px 10px;
            }
            QLineEdit:focus { border-color: #ff6b35; }
            QScrollBar:vertical { background: transparent; width: 4px; }
            QScrollBar::handle:vertical {
                background: rgba(255,107,53,0.4); border-radius: 2px;
            }
            QSplitter::handle { background: #1f2535; }
        )");
    }

    // ── Refresh ───────────────────────────────────────────────────────────────
    void refresh() {
        _entries = loadVaultEntries(_search->text().trimmed());

        // Apply type filter
        const int typeFilter = _typeFilter->currentIndex() - 1; // -1 = all
        if (typeFilter >= 0) {
            _entries.erase(
                std::remove_if(_entries.begin(), _entries.end(),
                    [typeFilter](const VaultEntry& e){ return e.type != typeFilter; }),
                _entries.end());
        }

        _list->clear();
        for (const auto& e : _entries) {
            const QString icon  = (e.type == 0) ? "🗑" : "⏱";
            const QString title = QString("%1 %2").arg(icon, e.peerName.isEmpty()
                ? "(unknown)" : e.peerName);
            const QString preview = e.text.left(60) + (e.text.size() > 60 ? "…" : "");
            const QString ts = e.timestamp.toString("dd.MM.yy hh:mm");

            auto* item = new QListWidgetItem;
            item->setText(
                title + "\n" +
                (!preview.isEmpty() ? preview : "📎 Media") + "\n" +
                ts);
            item->setData(Qt::UserRole, e.id);

            // Color coding
            if (e.type == 0) {
                item->setForeground(QColor(0xff, 0x6b, 0x35));
            } else {
                item->setForeground(QColor(0x55, 0xcc, 0xff));
            }
            _list->addItem(item);
        }

        _countLabel->setText(
            QString("  %1 saved message(s) in vault").arg(_entries.size()));

        if (!_entries.isEmpty()) _list->setCurrentRow(0);
        else clearDetail();
    }

    void showDetail(int row) {
        if (row < 0 || row >= _entries.size()) { clearDetail(); return; }
        const auto& e = _entries[row];

        const QString typeStr = (e.type == 0) ? "🗑 Deleted" : "⏱ Disappearing";
        _header->setText(typeStr + " — " + e.peerName);

        QString html = "<div style='font-family:monospace;'>";
        html += "<p><b style='color:#707890'>From:</b> <span style='color:#f0f2ff'>"
             + e.fromName.toHtmlEscaped() + "</span></p>";
        html += "<p><b style='color:#707890'>Chat:</b> <span style='color:#f0f2ff'>"
             + e.peerName.toHtmlEscaped() + "</span></p>";
        html += "<p><b style='color:#707890'>Sent:</b> <span style='color:#f0f2ff'>"
             + e.timestamp.toString("dd MMMM yyyy, hh:mm:ss") + "</span></p>";
        html += "<p><b style='color:#707890'>Saved:</b> <span style='color:#707890'>"
             + e.savedAt.toString("dd.MM.yyyy hh:mm") + "</span></p>";
        html += "<hr style='border:1px solid #1f2535;margin:8px 0'/>";

        if (!e.text.isEmpty()) {
            html += "<p style='color:#d0d8f0;white-space:pre-wrap;'>"
                 + e.text.toHtmlEscaped() + "</p>";
        }
        if (!e.mediaPath.isEmpty()) {
            html += "<p><b style='color:#ff6b35'>📎 Media:</b> <span style='color:#5bc5ff'>"
                 + e.mediaPath.toHtmlEscaped() + "</span></p>";
        }
        html += "</div>";

        _detail->setHtml(html);
        _deleteEntryBtn->setEnabled(true);
    }

    void clearDetail() {
        _header->setText("Select a message");
        _detail->clear();
        _deleteEntryBtn->setEnabled(false);
    }

    void deleteCurrentEntry() {
        const int row = _list->currentRow();
        if (row < 0 || row >= _entries.size()) return;
        const qint64 id = _entries[row].id;

        auto db = QSqlDatabase::database("FainsVaultDB");
        QSqlQuery q(db);
        q.prepare("DELETE FROM msgs WHERE id=?");
        q.addBindValue(id);
        q.exec();
        refresh();
    }

    void clearVault() {
        if (QMessageBox::question(this, "FainsGram Vault",
                "Delete ALL saved messages from the vault?\n"
                "This cannot be undone.")
            != QMessageBox::Yes) return;

        auto db = QSqlDatabase::database("FainsVaultDB");
        QSqlQuery("DELETE FROM msgs", db).exec();
        refresh();
    }

    void exportJson() {
        const QString path = QFileDialog::getSaveFileName(
            this, "Export Vault",
            QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)
                + "/fainsgram_vault.json",
            "JSON Files (*.json)");
        if (path.isEmpty()) return;

        QJsonArray arr;
        for (const auto& e : loadVaultEntries()) {
            QJsonObject o;
            o["id"]        = e.id;
            o["peer"]      = e.peerName;
            o["from"]      = e.fromName;
            o["text"]      = e.text;
            o["media"]     = e.mediaPath;
            o["timestamp"] = e.timestamp.toString(Qt::ISODate);
            o["saved_at"]  = e.savedAt.toString(Qt::ISODate);
            o["type"]      = (e.type == 0) ? "deleted" : "disappearing";
            arr.append(o);
        }

        QFile f(path);
        if (f.open(QIODevice::WriteOnly)) {
            f.write(QJsonDocument(arr).toJson());
            QMessageBox::information(this, "FainsGram",
                QString("Exported %1 messages to:\n%2").arg(arr.size()).arg(path));
        }
    }

    QListWidget*    _list          = nullptr;
    QTextBrowser*   _detail        = nullptr;
    QLabel*         _header        = nullptr;
    QLabel*         _countLabel    = nullptr;
    QLineEdit*      _search        = nullptr;
    QComboBox*      _typeFilter    = nullptr;
    QPushButton*    _deleteEntryBtn= nullptr;
    QVector<VaultEntry> _entries;
};

} // namespace FainsGram
