/*
 * FainsGram — Master Controller (Implementation)
 * Copyright (c) 2024 FainsGram. MIT License.
 */

#include "fainsgram_controller.h"
#include "ghost_controller.h"
#include "message_vault.h"
#include "local_premium.h"
#include "theme_engine.h"
#include "custom_badge.h"
#include "multi_account_manager.h"

#include <QtCore/QStandardPaths>
#include <QtCore/QDir>
#include <QtCore/QTimer>
#ifdef Q_OS_MAC
#include <malloc/malloc.h>
#endif

namespace FainsGram {

// ── Singleton ────────────────────────────────────────────────────────────────

FainsGramController& FainsGramController::instance() {
    static FainsGramController inst;
    return inst;
}

FainsGramController::FainsGramController(QObject* parent) : QObject(parent) {
    const QString configDir = QStandardPaths::writableLocation(
        QStandardPaths::AppConfigLocation) + "/FainsGram";
    QDir().mkpath(configDir);

    _settings = std::make_unique<QSettings>(
        configDir + "/fainsgram.ini", QSettings::IniFormat);
}

FainsGramController::~FainsGramController() {
    shutdown();
}

// ── Lifecycle ────────────────────────────────────────────────────────────────

void FainsGramController::initialize() {
    if (_initialized) return;

    qDebug("[FainsGram] Initializing v%s ...", kForkVersion);

    _ghost    = std::make_unique<GhostController>(this);
    _vault    = std::make_unique<MessageVault>(this);
    _premium  = std::make_unique<LocalPremium>(this);
    _theme    = std::make_unique<ThemeEngine>(this);
    _badge    = std::make_unique<CustomBadge>(this);
    _accounts = std::make_unique<MultiAccountManager>(this);

    // Boot order matters
    _ghost->loadSettings();
    _premium->loadSettings();
    _premium->apply();          // Patch local user object early
    _theme->loadSettings();
    _theme->apply();            // Apply fonts / wallpaper / theme
    _badge->loadSettings();
    _accounts->initialize();    // Unlock account limit
    _vault->initialize();       // Open SQLite vault DB

    // Periodic RAM optimization
    auto memTimer = new QTimer(this);
    connect(memTimer, &QTimer::timeout, [=] {
#ifdef Q_OS_MAC
        malloc_zone_pressure_relief(nullptr, 0);
#endif
    });
    memTimer->start(30000);

    _initialized = true;
    qDebug("[FainsGram] Ready.");
    Q_EMIT initialized();
}

void FainsGramController::shutdown() {
    if (!_initialized) return;

    _vault->exportToiCloudIfEnabled();
    _settings->sync();
    _initialized = false;
    qDebug("[FainsGram] Shutdown complete.");
}

} // namespace FainsGram
