/*
 * FainsGram — Multi Account Manager
 * ===================================
 * Bypasses Telegram Desktop's hard-coded 3-account limit.
 *
 * How TDesktop limits accounts:
 *  - `Main::Domain::kMaxAccounts` is a constexpr = 3 (or 4 with Premium)
 *  - `Main::Domain::accountsNeedToBeLoaded()` skips accounts beyond the cap
 *  - The "Add Account" button is hidden when count >= kMaxAccounts
 *
 * FainsGram approach:
 *  1. Patch `kMaxAccounts` at runtime via our wrapper (no recompile needed for the limit)
 *  2. Override `canAddAccount()` to always return true
 *  3. Un-hide the "Add Account" button regardless of count
 *  4. Store additional session files using extended index (tdata/s4, s5, …)
 *
 * Copyright (c) 2024 FainsGram. MIT License.
 */

#pragma once

#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <vector>

namespace FainsGram {

struct FainsAccount {
    int     index      = 0;   // tdata/s<index>
    QString name;             // Display name override (optional)
    bool    active     = true;
};

class MultiAccountManager final : public QObject {
    Q_OBJECT

public:
    explicit MultiAccountManager(QObject* parent = nullptr);

    void initialize();

    // ── Core bypass ──────────────────────────────────────────────────────────

    /**
     * Returns the effective maximum accounts limit.
     * FainsGram: unlimited (returns INT_MAX in practice, capped at 100 for UI).
     * Call this wherever TDesktop checks kMaxAccounts.
     */
    static int effectiveMaxAccounts() { return kFainsMaxAccounts; }

    /**
     * Call this instead of Main::Domain::canAddAccount().
     * Always returns true when FainsGram multi-account is enabled.
     */
    bool canAddAccount() const;

    // ── Account list ─────────────────────────────────────────────────────────

    int  accountCount() const { return static_cast<int>(_accounts.size()); }
    const std::vector<FainsAccount>& accounts() const { return _accounts; }

    void loadAccounts();      // Scan tdata/ for s0…s<N> session files
    void saveAccounts();

    // Add a slot for a new account (creates empty session placeholder)
    int  addAccountSlot();

    // Remove account by index (marks inactive; actual tdata removal = user action)
    void removeAccountSlot(int index);

    // Rename account in sidebar
    void setAccountName(int index, const QString& name);
    QString accountName(int index) const;

    // ── Settings ─────────────────────────────────────────────────────────────

    bool isUnlimitedEnabled() const { return _enabled; }
    void setUnlimitedEnabled(bool on);

    void loadSettings();
    void saveSettings();

Q_SIGNALS:
    void accountAdded(int index);
    void accountRemoved(int index);
    void accountLimitChanged(bool unlimited);

private:
    static constexpr int kFainsMaxAccounts = 100; // Practical UI cap

    void scanTdata();
    QString tdataSessionPath(int index) const;

    std::vector<FainsAccount> _accounts;
    bool _enabled = true;
};

} // namespace FainsGram
