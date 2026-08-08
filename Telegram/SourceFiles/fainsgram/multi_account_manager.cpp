/*
 * FainsGram — Multi Account Manager (Implementation)
 * Copyright (c) 2024 FainsGram. MIT License.
 */

#include "multi_account_manager.h"
#include "fainsgram_controller.h"

#include <QtCore/QDir>
#include <QtCore/QStandardPaths>
#include <QtCore/QSettings>
#include <QtCore/QFileInfo>
#include <climits>

namespace FainsGram {

MultiAccountManager::MultiAccountManager(QObject* parent) : QObject(parent) {}

// ── Initialize ───────────────────────────────────────────────────────────────

void MultiAccountManager::initialize() {
    loadSettings();
    loadAccounts();

    if (_enabled) {
        qDebug("[FainsGram] MultiAccount: unlimited mode ON, found %d accounts",
               accountCount());
    }
}

// ── Account scanning ─────────────────────────────────────────────────────────

/**
 * Telegram Desktop stores each logged-in account session as:
 *   tdata/              (account index 0 — primary, no suffix)
 *   tdata/s1/           (account index 1)
 *   tdata/s2/           (account index 2)
 *   ...
 *
 * We scan for all such directories up to kFainsMaxAccounts.
 */
void MultiAccountManager::loadAccounts() {
    _accounts.clear();

    // Account 0 — always present if tdata/ exists
    const QString tdataBase = QStandardPaths::writableLocation(
        QStandardPaths::AppLocalDataLocation) + "/tdata";

    if (QDir(tdataBase).exists()) {
        FainsAccount a0;
        a0.index  = 0;
        a0.name   = accountName(0);
        a0.active = true;
        _accounts.push_back(a0);
    }

    // Accounts 1..N — look for tdata/s<N>/
    for (int i = 1; i < kFainsMaxAccounts; ++i) {
        const QString path = tdataBase + "/s" + QString::number(i);
        if (QDir(path).exists()) {
            FainsAccount a;
            a.index  = i;
            a.name   = accountName(i);
            a.active = true;
            _accounts.push_back(a);
        }
    }
}

void MultiAccountManager::saveAccounts() {
    auto& s = FG().settings();
    s.beginGroup("MultiAccount");
    s.setValue("count", accountCount());
    for (const auto& a : _accounts) {
        s.setValue(QString("name_%1").arg(a.index), a.name);
    }
    s.endGroup();
}

int MultiAccountManager::addAccountSlot() {
    // Find next free index
    int nextIndex = 0;
    for (const auto& a : _accounts) {
        if (a.index >= nextIndex) nextIndex = a.index + 1;
    }

    // Create placeholder directory so TDesktop picks it up
    const QString tdataBase = QStandardPaths::writableLocation(
        QStandardPaths::AppLocalDataLocation) + "/tdata";
    const QString newPath = (nextIndex == 0)
        ? tdataBase
        : tdataBase + "/s" + QString::number(nextIndex);
    QDir().mkpath(newPath);

    FainsAccount a;
    a.index  = nextIndex;
    a.active = true;
    _accounts.push_back(a);
    saveAccounts();

    Q_EMIT accountAdded(nextIndex);
    qDebug("[FainsGram] MultiAccount: added slot %d at %s",
           nextIndex, qPrintable(newPath));
    return nextIndex;
}

void MultiAccountManager::removeAccountSlot(int index) {
    _accounts.erase(
        std::remove_if(_accounts.begin(), _accounts.end(),
            [index](const FainsAccount& a){ return a.index == index; }),
        _accounts.end());
    saveAccounts();
    Q_EMIT accountRemoved(index);
}

// ── Core bypass ──────────────────────────────────────────────────────────────

bool MultiAccountManager::canAddAccount() const {
    if (!_enabled) {
        // Fallback to TDesktop default (3 or 4 with real premium)
        return accountCount() < 3;
    }
    // FainsGram: always allow up to 100 accounts
    return accountCount() < kFainsMaxAccounts;
}

// ── Name overrides ───────────────────────────────────────────────────────────

void MultiAccountManager::setAccountName(int index, const QString& name) {
    for (auto& a : _accounts) {
        if (a.index == index) { a.name = name; break; }
    }
    saveAccounts();
}

QString MultiAccountManager::accountName(int index) const {
    for (const auto& a : _accounts) {
        if (a.index == index) return a.name;
    }
    auto& s = FG().settings();
    return s.value(
        QString("MultiAccount/name_%1").arg(index),
        QString("Account %1").arg(index + 1)).toString();
}

// ── Settings ─────────────────────────────────────────────────────────────────

void MultiAccountManager::loadSettings() {
    auto& s = FG().settings();
    s.beginGroup("MultiAccount");
    _enabled = s.value("unlimited", true).toBool();
    s.endGroup();
}

void MultiAccountManager::saveSettings() {
    auto& s = FG().settings();
    s.beginGroup("MultiAccount");
    s.setValue("unlimited", _enabled);
    s.endGroup();
    s.sync();
}

void MultiAccountManager::setUnlimitedEnabled(bool on) {
    if (_enabled == on) return;
    _enabled = on;
    saveSettings();
    Q_EMIT accountLimitChanged(on);
}

QString MultiAccountManager::tdataSessionPath(int index) const {
    const QString tdataBase = QStandardPaths::writableLocation(
        QStandardPaths::AppLocalDataLocation) + "/tdata";
    return (index == 0) ? tdataBase : tdataBase + "/s" + QString::number(index);
}

} // namespace FainsGram
