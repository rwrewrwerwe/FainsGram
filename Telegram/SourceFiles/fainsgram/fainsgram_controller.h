/*
 * FainsGram — Custom Telegram Desktop Fork
 * ==========================================
 * Master Controller: owns and coordinates all fork modules.
 *
 * Modules:
 *  - GhostController      Ghost Mode (no read receipts, hide online, ghost stories)
 *  - MessageVault         Save deleted / disappearing messages
 *  - LocalPremium         Local premium unlock + no-ads
 *  - ThemeEngine          Custom wallpaper, font, icon, themes
 *  - CustomBadge          FainsGram badge for fork users
 *  - MultiAccountManager  Unlimited accounts (bypass 3-account limit)
 *
 * Copyright (c) 2024 FainsGram. MIT License.
 */

#pragma once

#include <memory>
#include <QtCore/QObject>
#include <QtCore/QSettings>

// Forward declarations
namespace FainsGram {

class GhostController;
class MessageVault;
class LocalPremium;
class ThemeEngine;
class CustomBadge;
class MultiAccountManager;

// ─────────────────────────────────────────────────────────────────────────────
class FainsGramController final : public QObject {
    Q_OBJECT

public:
    static FainsGramController& instance();
    ~FainsGramController() override;

    // Call once at app startup (before main window is shown)
    void initialize();
    void shutdown();

    bool isInitialized() const { return _initialized; }

    // Sub-system accessors
    GhostController*      ghost()    const { if (!_initialized) const_cast<FainsGramController*>(this)->initialize(); return _ghost.get(); }
    MessageVault*         vault()    const { if (!_initialized) const_cast<FainsGramController*>(this)->initialize(); return _vault.get(); }
    LocalPremium*         premium()  const { if (!_initialized) const_cast<FainsGramController*>(this)->initialize(); return _premium.get(); }
    ThemeEngine*          theme()    const { if (!_initialized) const_cast<FainsGramController*>(this)->initialize(); return _theme.get(); }
    CustomBadge*          badge()    const { if (!_initialized) const_cast<FainsGramController*>(this)->initialize(); return _badge.get(); }
    MultiAccountManager*  accounts() const { if (!_initialized) const_cast<FainsGramController*>(this)->initialize(); return _accounts.get(); }

    // Persistent config (INI, stored in app data dir)
    QSettings& settings() { return *_settings; }

    // Fork metadata
    static constexpr const char* kForkName    = "FainsGram";
    static constexpr const char* kForkVersion = "1.0.0";
    static constexpr const char* kForkBadge   = "⚡";

Q_SIGNALS:
    void initialized();

private:
    explicit FainsGramController(QObject* parent = nullptr);

    std::unique_ptr<GhostController>     _ghost;
    std::unique_ptr<MessageVault>        _vault;
    std::unique_ptr<LocalPremium>        _premium;
    std::unique_ptr<ThemeEngine>         _theme;
    std::unique_ptr<CustomBadge>         _badge;
    std::unique_ptr<MultiAccountManager> _accounts;
    std::unique_ptr<QSettings>           _settings;

    bool _initialized = false;
};

} // namespace FainsGram

// Global convenience macro — use FG().ghost(), FG().vault(), etc.
#define FG() FainsGram::FainsGramController::instance()
