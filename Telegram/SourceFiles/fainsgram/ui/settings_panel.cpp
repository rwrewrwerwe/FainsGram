/*
 * FainsGram — Settings Panel UI (Implementation)
 * Copyright (c) 2024 FainsGram. MIT License.
 */

#include "settings_panel.h"
#include "settings/settings_common_session.h"
#include "core/application.h"
#include "core/version.h"
#include "ui/layers/generic_box.h"
#include "ui/wrap/vertical_layout.h"
#include "ui/vertical_list.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/box_content_divider.h"
#include "ui/widgets/fields/input_field.h"
#include "main/main_session.h"
#include "data/data_changes.h"
#include "data/data_user.h"
#include "window/window_session_controller.h"
#include "lang/lang_keys.h"
#include "styles/style_settings.h"
#include "styles/style_premium.h"
#include "../fainsgram_controller.h"
#include "../ghost_controller.h"
#include "../message_vault.h"
#include "../local_premium.h"
#include "../theme_engine.h"
#include "../custom_badge.h"
#include "../multi_account_manager.h"
#include "../ws_proxy_controller.h"

#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QFontComboBox>
#include <QtWidgets/QColorDialog>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QApplication>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QLinearGradient>
#include <QtCore/QEasingCurve>
#include <QtGui/QDesktopServices>
#include <QtCore/QUrl>
#include <QtCore/QRandomGenerator>

namespace FainsGram {

// ─────────────────────────────────────────────────────────────────────────────
// ToggleSwitch
// ─────────────────────────────────────────────────────────────────────────────

ToggleSwitch::ToggleSwitch(QWidget* parent) : QWidget(parent) {
    setFixedSize(52, 28);
    setCursor(Qt::PointingHandCursor);
    _anim = new QPropertyAnimation(this, "offset", this);
    _anim->setDuration(180);
    _anim->setEasingCurve(QEasingCurve::OutCubic);
}

void ToggleSwitch::setChecked(bool on) {
    if (_checked == on) return;
    _checked = on;
    _anim->stop();
    _anim->setStartValue(_offset);
    _anim->setEndValue(on ? 28 : 4);
    _anim->start();
    Q_EMIT toggled(on);
}

void ToggleSwitch::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Track
    QColor track = _checked ? QColor(0xff, 0x6b, 0x35) : QColor(0x40, 0x44, 0x55);
    p.setBrush(track);
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(rect(), 14, 14);

    // Thumb
    p.setBrush(Qt::white);
    p.drawEllipse(_offset, 4, 20, 20);
}

void ToggleSwitch::mousePressEvent(QMouseEvent*) {
    toggle();
}

// ─────────────────────────────────────────────────────────────────────────────
// SectionButton
// ─────────────────────────────────────────────────────────────────────────────

SectionButton::SectionButton(const QString& icon, const QString& label, QWidget* parent)
    : QPushButton(parent) {
    setText(icon + "\n" + label);
    setCheckable(true);
    setFixedHeight(60);
    setStyleSheet(R"(
        QPushButton {
            border: none;
            background: transparent;
            color: #707890;
            font-size: 10px;
            padding: 6px 4px;
            border-radius: 8px;
        }
        QPushButton:checked, QPushButton:hover {
            background: rgba(255,107,53,0.15);
            color: #ff6b35;
        }
    )");
}

void SectionButton::setActive(bool on) {
    setChecked(on);
}

// ─────────────────────────────────────────────────────────────────────────────
// FainsGramSettingsPanel
// ─────────────────────────────────────────────────────────────────────────────

FainsGramSettingsPanel::FainsGramSettingsPanel(QWidget* parent) : Ui::RpWidget(parent) {
    setObjectName("FainsGramSettings");
    _mainLayout = new QVBoxLayout(this);
    _mainLayout->setContentsMargins(0, 0, 0, 0);
    _mainLayout->setSpacing(0);

    buildHeader();
    buildTabBar();
    buildPages();
    applyStyle();
}

void FainsGramSettingsPanel::paintEvent(QPaintEvent* e) {
    QWidget::paintEvent(e);
}

void FainsGramSettingsPanel::applyStyle() {
    setStyleSheet(R"(
        #FainsGramSettings {
            background-color: #171d27;
        }
        QScrollArea { background: transparent; border: none; }
        QScrollBar:vertical {
            background: transparent; width: 4px;
        }
        QScrollBar::handle:vertical {
            background: rgba(255,107,53,0.4);
            border-radius: 2px;
        }
        QLabel { color: #f0f2ff; background: transparent; }
        QLineEdit {
            background: #1f2535;
            border: 1px solid #2a3045;
            border-radius: 8px;
            color: #f0f2ff;
            padding: 8px 12px;
            font-size: 13px;
        }
        QLineEdit:focus { border-color: #ff6b35; }
        QPushButton#actionBtn {
            background: #ff6b35;
            color: white;
            border: none;
            border-radius: 8px;
            padding: 8px 20px;
            font-weight: bold;
        }
        QPushButton#actionBtn:hover { background: #ff8555; }
        QPushButton#dangerBtn {
            background: rgba(255,60,60,0.15);
            color: #ff5555;
            border: 1px solid rgba(255,60,60,0.3);
            border-radius: 8px;
            padding: 8px 20px;
        }
        QComboBox {
            background: #1f2535;
            border: 1px solid #2a3045;
            border-radius: 8px;
            color: #f0f2ff;
            padding: 6px 12px;
        }
        QComboBox::drop-down { border: none; }
    )");
}

// ── Header ───────────────────────────────────────────────────────────────────

void FainsGramSettingsPanel::buildHeader() {
    _header = new QWidget;
    _header->setFixedHeight(100);
    _header->setStyleSheet(R"(
        background: qlineargradient(x1:0,y1:0,x2:1,y2:1,
            stop:0 #1a2035, stop:0.5 #1f1528, stop:1 #1a2035);
        border-bottom: 1px solid rgba(255,107,53,0.2);
    )");

    auto* hlay = new QHBoxLayout(_header);
    hlay->setContentsMargins(20, 16, 20, 16);

    // Icon
    auto* icon = new QLabel("⚡");
    icon->setStyleSheet("font-size: 36px; background: transparent;");

    // Title block
    auto* titleBlock = new QVBoxLayout;
    auto* title = new QLabel("FainsGram");
    title->setStyleSheet(
        "font-size: 20px; font-weight: bold; color: #ff6b35; background: transparent;");
    auto* sub = new QLabel(QString("v%1 · Custom Fork")
                           .arg(FainsGramController::kForkVersion));
    sub->setStyleSheet("font-size: 12px; color: #707890; background: transparent;");
    titleBlock->addWidget(title);
    titleBlock->addWidget(sub);
    titleBlock->setSpacing(2);

    hlay->addWidget(icon);
    hlay->addSpacing(12);
    hlay->addLayout(titleBlock);
    hlay->addStretch();

    // Ghost mode quick toggle
    auto* ghostLabel = new QLabel("Ghost");
    ghostLabel->setStyleSheet("font-size: 11px; color: #707890; background:transparent;");
    auto* ghostToggle = new ToggleSwitch;
    // Capture the pointer once and null-guard every deref. FG().ghost() can
    // return nullptr before the controller finishes lazy initialization, and a
    // non-null-but-stale pointer would fault on ->isEnabled() (EXC_BAD_ACCESS).
    if (auto* ghost = FG().ghost()) {
        ghostToggle->setChecked(ghost->isEnabled());
        connect(ghostToggle, &ToggleSwitch::toggled,
                [ghost](bool on){ ghost->setEnabled(on); });
        connect(ghost, &GhostController::ghostModeChanged,
                ghostToggle, &ToggleSwitch::setChecked);
    }

    auto* quickBox = new QVBoxLayout;
    quickBox->addWidget(ghostLabel, 0, Qt::AlignCenter);
    quickBox->addWidget(ghostToggle);
    quickBox->setSpacing(4);

    hlay->addLayout(quickBox);

    _mainLayout->addWidget(_header);
}

// ── Tab bar ───────────────────────────────────────────────────────────────────

void FainsGramSettingsPanel::buildTabBar() {
    auto* bar = new QWidget;
    bar->setFixedHeight(72);
    bar->setStyleSheet("background: #10141d; border-bottom: 1px solid #1f2535;");

    _tabBar = new QHBoxLayout(bar);
    _tabBar->setContentsMargins(8, 6, 8, 6);
    _tabBar->setSpacing(2);

    const QVector<QPair<QString,QString>> tabs = {
        {"👻", "Ghost"},
        {"🗄", "Vault"},
        {"👤", "Accounts"},
        {"🎨", "Theme"},
        {"⭐", "Premium"},
        {"ℹ️", "About"},
    };

    for (int i = 0; i < tabs.size(); ++i) {
        auto* btn = new SectionButton(tabs[i].first, tabs[i].second, bar);
        _tabs.append(btn);
        _tabBar->addWidget(btn);
        connect(btn, &QPushButton::clicked, [this, i]{ switchToPage(i); });
    }
    _tabs[0]->setActive(true);

    _mainLayout->addWidget(bar);
}

void FainsGramSettingsPanel::switchToPage(int idx) {
    for (int i = 0; i < _tabs.size(); ++i)
        _tabs[i]->setActive(i == idx);
    _pages->setCurrentIndex(idx);
    _currentPage = idx;
}

// ── Pages ────────────────────────────────────────────────────────────────────

void FainsGramSettingsPanel::buildPages() {
    _pages = new QStackedWidget;
    _pages->addWidget(buildGhostPage());
    _pages->addWidget(buildVaultPage());
    _pages->addWidget(buildAccountsPage());
    _pages->addWidget(buildThemePage());
    _pages->addWidget(buildPremiumPage());
    _pages->addWidget(buildAboutPage());
    _mainLayout->addWidget(_pages, 1);
}

// Helper widgets
QWidget* FainsGramSettingsPanel::makeRow(
        const QString& title, const QString& subtitle, QWidget* control) {

    auto* row = new QWidget;
    row->setFixedHeight(58);
    row->setStyleSheet(
        "QWidget { background: #13172280; border-radius: 10px; }"
        "QWidget:hover { background: #1a1f2d80; }");

    auto* lay = new QHBoxLayout(row);
    lay->setContentsMargins(16, 0, 16, 0);

    auto* textBlock = new QVBoxLayout;
    auto* t = new QLabel(title);
    t->setStyleSheet("font-size: 13px; font-weight: 600; color: #f0f2ff;");
    textBlock->addWidget(t);
    if (!subtitle.isEmpty()) {
        auto* s = new QLabel(subtitle);
        s->setStyleSheet("font-size: 11px; color: #707890;");
        textBlock->addWidget(s);
    }
    textBlock->setSpacing(2);

    lay->addLayout(textBlock, 1);
    if (control) lay->addWidget(control);
    return row;
}

QWidget* FainsGramSettingsPanel::makeSeparator() {
    auto* w = new QWidget;
    w->setFixedHeight(1);
    w->setStyleSheet("background: #1f2535;");
    return w;
}

QWidget* FainsGramSettingsPanel::makeSectionHeader(const QString& text) {
    auto* lbl = new QLabel(text.toUpper());
    lbl->setContentsMargins(16, 12, 16, 4);
    lbl->setStyleSheet("font-size: 11px; font-weight: bold; color: #ff6b35; letter-spacing: 1px;");
    return lbl;
}

// ── Ghost page ────────────────────────────────────────────────────────────────

QWidget* FainsGramSettingsPanel::buildGhostPage() {
    auto* scroll = new QScrollArea;
    auto* content = new QWidget;
    auto* lay = new QVBoxLayout(content);
    lay->setContentsMargins(12, 12, 12, 12);
    lay->setSpacing(6);

    if (!FG().isInitialized() || !FG().ghost()) {
        auto* warn = new QLabel("FainsGram not initialized yet.\nPlease restart the app.");
        warn->setAlignment(Qt::AlignCenter);
        warn->setStyleSheet("color: #707890; font-size: 13px; margin: 40px;");
        lay->addWidget(warn);
        scroll->setWidget(content);
        scroll->setWidgetResizable(true);
        return scroll;
    }
    auto* ghost = FG().ghost();

    // Master toggle
    auto* masterToggle = new ToggleSwitch;
    masterToggle->setChecked(ghost->isEnabled());
    connect(masterToggle, &ToggleSwitch::toggled,
            ghost, &GhostController::setEnabled);
    connect(ghost, &GhostController::ghostModeChanged,
            masterToggle, &ToggleSwitch::setChecked);

    lay->addWidget(makeSectionHeader("Ghost Mode"));
    lay->addWidget(makeRow("Ghost Mode",
        "Enable all ghost features at once", masterToggle));
    lay->addWidget(makeSeparator());
    lay->addWidget(makeSectionHeader("Individual Controls"));

    struct Row { QString title, sub; bool (GhostController::*get)() const;
                 void (GhostController::*set)(bool); };
    const QVector<Row> rows = {
        {"No Read Receipts", "Messages won't show blue ticks",
         &GhostController::noReadReceipts,    &GhostController::setNoReadReceipts},
        {"Hide Online Status", "Always appear offline to everyone",
         &GhostController::hideOnline,        &GhostController::setHideOnline},
        {"Ghost Stories", "View stories without appearing in viewers",
         &GhostController::ghostStories,      &GhostController::setGhostStories},
        {"No Typing Indicator", "Don't show 'typing…' to others",
         &GhostController::noTypingIndicator, &GhostController::setNoTypingIndicator},
        {"Hide Last Seen", "Force 'last seen recently' for everyone",
         &GhostController::hideLastSeen,      &GhostController::setHideLastSeen},
    };

    for (const auto& r : rows) {
        auto* t = new ToggleSwitch;
        t->setChecked((ghost->*r.get)());
        connect(t, &ToggleSwitch::toggled, ghost, r.set);
        lay->addWidget(makeRow(r.title, r.sub, t));
    }

    lay->addStretch();
    scroll->setWidget(content);
    scroll->setWidgetResizable(true);
    return scroll;
}

// ── Vault page ────────────────────────────────────────────────────────────────

QWidget* FainsGramSettingsPanel::buildVaultPage() {
    auto* scroll = new QScrollArea;
    auto* content = new QWidget;
    auto* lay = new QVBoxLayout(content);
    lay->setContentsMargins(12, 12, 12, 12);
    lay->setSpacing(6);

    if (!FG().isInitialized() || !FG().vault()) {
        lay->addWidget(new QLabel("FainsGram not initialized."));
        scroll->setWidget(content); scroll->setWidgetResizable(true); return scroll;
    }
    auto* vault = FG().vault();

    lay->addWidget(makeSectionHeader("Message Vault"));

    // iCloud sync
    auto& cfg = FG().settings();
    auto* syncToggle = new ToggleSwitch;
    syncToggle->setChecked(cfg.value("Vault/iCloudSync", false).toBool());
    connect(syncToggle, &ToggleSwitch::toggled, [](bool on){
        FG().settings().setValue("Vault/iCloudSync", on);
    });
    lay->addWidget(makeRow("iCloud Sync",
        "Auto-export vault to iCloud Drive (macOS)", syncToggle));

    // Export button
    auto* exportBtn = new QPushButton("Export Vault to Folder");
    exportBtn->setObjectName("actionBtn");
    connect(exportBtn, &QPushButton::clicked, [vault, this]{
        QString dir = QFileDialog::getExistingDirectory(
            this, "Choose Export Folder");
        if (!dir.isEmpty()) {
            vault->exportToPath(dir);
            QMessageBox::information(this, "FainsGram", "Vault exported to:\n" + dir);
        }
    });
    lay->addWidget(makeRow("Export Vault", "Save to a local folder", exportBtn));

    // Clear button
    auto* clearBtn = new QPushButton("Clear Vault");
    clearBtn->setObjectName("dangerBtn");
    connect(clearBtn, &QPushButton::clicked, [vault, this]{
        auto r = QMessageBox::question(this, "FainsGram",
            "Delete all saved messages from the vault?");
        if (r == QMessageBox::Yes) vault->clear();
    });
    lay->addWidget(makeRow("Clear All", "Remove all captured messages", clearBtn));

    lay->addStretch();
    scroll->setWidget(content);
    scroll->setWidgetResizable(true);
    return scroll;
}

// ── Accounts page ─────────────────────────────────────────────────────────────

QWidget* FainsGramSettingsPanel::buildAccountsPage() {
    auto* scroll = new QScrollArea;
    auto* content = new QWidget;
    auto* lay = new QVBoxLayout(content);
    lay->setContentsMargins(12, 12, 12, 12);
    lay->setSpacing(6);

    if (!FG().isInitialized() || !FG().accounts()) {
        lay->addWidget(new QLabel("FainsGram not initialized."));
        scroll->setWidget(content); scroll->setWidgetResizable(true); return scroll;
    }
    auto* mgr = FG().accounts();

    lay->addWidget(makeSectionHeader("Multi-Account"));

    auto* unlimitedToggle = new ToggleSwitch;
    unlimitedToggle->setChecked(mgr->isUnlimitedEnabled());
    connect(unlimitedToggle, &ToggleSwitch::toggled,
            mgr, &MultiAccountManager::setUnlimitedEnabled);
    lay->addWidget(makeRow("Unlimited Accounts",
        "Remove 3-account limit (no Premium needed)", unlimitedToggle));

    // Account count info
    auto* countLabel = new QLabel(
        QString("Currently logged in: %1 account(s)").arg(mgr->accountCount()));
    countLabel->setStyleSheet("color: #707890; font-size: 12px; margin-left: 16px;");
    lay->addWidget(countLabel);

    // Add account button
    auto* addBtn = new QPushButton("＋ Add Another Account");
    addBtn->setObjectName("actionBtn");
    connect(addBtn, &QPushButton::clicked, [mgr, countLabel]{
        if (mgr->canAddAccount()) {
            int idx = mgr->addAccountSlot();
            countLabel->setText(
                QString("Currently logged in: %1 account(s)").arg(mgr->accountCount()));
            QMessageBox::information(nullptr, "FainsGram",
                QString("Account slot %1 created.\n"
                        "Restart FainsGram to log in to the new account.")
                .arg(idx));
        }
    });
    lay->addWidget(makeRow("Add Account",
        "No Telegram Premium required", addBtn));

    lay->addWidget(makeSectionHeader("Active Accounts"));

    // List all accounts
    for (const auto& a : mgr->accounts()) {
        auto* renameBtn = new QPushButton("Rename");
        renameBtn->setObjectName("actionBtn");
        renameBtn->setFixedWidth(80);
        int idx = a.index;
        connect(renameBtn, &QPushButton::clicked, [mgr, idx, this]{
            bool ok;
            QString name = QInputDialog::getText(
                this, "FainsGram",
                "New name for account " + QString::number(idx + 1) + ":",
                QLineEdit::Normal, mgr->accountName(idx), &ok);
            if (ok && !name.isEmpty()) mgr->setAccountName(idx, name);
        });
        QString displayName = a.name.isEmpty()
            ? QString("Account %1").arg(a.index + 1)
            : a.name;
        lay->addWidget(makeRow(displayName,
            QString("Session s%1").arg(a.index), renameBtn));
    }

    lay->addStretch();
    scroll->setWidget(content);
    scroll->setWidgetResizable(true);
    return scroll;
}

// ── Theme page ────────────────────────────────────────────────────────────────

QWidget* FainsGramSettingsPanel::buildThemePage() {
    auto* scroll = new QScrollArea;
    auto* content = new QWidget;
    auto* lay = new QVBoxLayout(content);
    lay->setContentsMargins(12, 12, 12, 12);
    lay->setSpacing(6);

    if (!FG().isInitialized() || !FG().theme()) {
        lay->addWidget(new QLabel("FainsGram not initialized."));
        scroll->setWidget(content); scroll->setWidgetResizable(true); return scroll;
    }
    auto* theme = FG().theme();

    // ── Built-in themes
    lay->addWidget(makeSectionHeader("Built-in Themes"));

    auto* themeCombo = new QComboBox;
    themeCombo->addItems({"FainsGram Default", "Ghost Dark", "Neon",
                           "Minimal", "AMOLED"});
    themeCombo->setCurrentIndex((int)theme->currentTheme());
    connect(themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [theme](int idx){ theme->applyBuiltinTheme((BuiltinTheme)idx); });
    lay->addWidget(makeRow("Theme", "Choose built-in colour scheme", themeCombo));

    // ── Wallpaper
    lay->addWidget(makeSectionHeader("Wallpaper"));

    auto* wallBtn = new QPushButton("Choose Wallpaper…");
    wallBtn->setObjectName("actionBtn");
    connect(wallBtn, &QPushButton::clicked, [theme, this]{
        QString path = QFileDialog::getOpenFileName(
            this, "Choose Wallpaper",
            QDir::homePath(),
            "Images (*.png *.jpg *.jpeg *.webp *.gif)");
        if (!path.isEmpty()) theme->setWallpaper(path);
    });
    lay->addWidget(makeRow("Set Wallpaper",
        "PNG, JPG, WebP or animated GIF", wallBtn));

    auto* blurToggle = new ToggleSwitch;
    blurToggle->setChecked(theme->settings().wallpaperBlur);
    connect(blurToggle, &ToggleSwitch::toggled, theme, &ThemeEngine::setWallpaperBlur);
    lay->addWidget(makeRow("Blur Wallpaper", "Apply gaussian blur", blurToggle));

    auto* clearWallBtn = new QPushButton("Remove Wallpaper");
    clearWallBtn->setObjectName("dangerBtn");
    connect(clearWallBtn, &QPushButton::clicked,
            theme, &ThemeEngine::clearWallpaper);
    lay->addWidget(makeRow("Clear Wallpaper", "", clearWallBtn));

    // ── Font
    lay->addWidget(makeSectionHeader("Font"));

    auto* fontCombo = new QFontComboBox;
    fontCombo->setCurrentFont(QFont(theme->settings().fontFamily));
    connect(fontCombo, &QFontComboBox::currentFontChanged, [theme](const QFont& f){
        theme->setCustomFont(f.family());
    });
    lay->addWidget(makeRow("Font Family", "Applied app-wide", fontCombo));

    auto* fontFileBtn = new QPushButton("Load Font File…");
    fontFileBtn->setObjectName("actionBtn");
    connect(fontFileBtn, &QPushButton::clicked, [theme, this]{
        QString path = QFileDialog::getOpenFileName(
            this, "Load Font",
            QDir::homePath(),
            "Fonts (*.ttf *.otf)");
        if (!path.isEmpty()) theme->setCustomFontFile(path);
    });
    lay->addWidget(makeRow("Custom Font File",
        "Load .ttf / .otf font", fontFileBtn));

    // ── App icon (macOS)
    lay->addWidget(makeSectionHeader("App Icon (macOS)"));

    auto* iconBtn = new QPushButton("Set Custom Icon…");
    iconBtn->setObjectName("actionBtn");
    connect(iconBtn, &QPushButton::clicked, [theme, this]{
        QString path = QFileDialog::getOpenFileName(
            this, "Choose Icon",
            QDir::homePath(),
            "Icons (*.icns *.png)");
        if (!path.isEmpty()) theme->setAppIcon(path);
    });
    lay->addWidget(makeRow("App Icon",
        "Swap dock icon without recompiling", iconBtn));

    // ── Import theme file
    lay->addWidget(makeSectionHeader("Import Theme"));
    auto* themeFileBtn = new QPushButton("Import .tdesktop-theme…");
    themeFileBtn->setObjectName("actionBtn");
    connect(themeFileBtn, &QPushButton::clicked, [theme, this]{
        QString path = QFileDialog::getOpenFileName(
            this, "Import Theme",
            QDir::homePath(),
            "Telegram Themes (*.tdesktop-theme *.tdesktop-palette)");
        if (!path.isEmpty()) theme->importThemeFile(path);
    });
    lay->addWidget(makeRow("Theme File",
        "Standard Telegram Desktop themes", themeFileBtn));

    lay->addStretch();
    scroll->setWidget(content);
    scroll->setWidgetResizable(true);
    return scroll;
}

// ── Premium page ──────────────────────────────────────────────────────────────

QWidget* FainsGramSettingsPanel::buildPremiumPage() {
    auto* scroll = new QScrollArea;
    auto* content = new QWidget;
    auto* lay = new QVBoxLayout(content);
    lay->setContentsMargins(12, 12, 12, 12);
    lay->setSpacing(6);

    if (!FG().isInitialized() || !FG().premium() || !FG().badge()) {
        lay->addWidget(new QLabel("FainsGram not initialized."));
        scroll->setWidget(content); scroll->setWidgetResizable(true); return scroll;
    }
    auto* prem = FG().premium();

    lay->addWidget(makeSectionHeader("Local Premium"));

    auto* premToggle = new ToggleSwitch;
    premToggle->setChecked(prem->isEnabled());
    connect(premToggle, &ToggleSwitch::toggled, prem, &LocalPremium::setEnabled);
    lay->addWidget(makeRow("Enable Local Premium",
        "Unlock premium UI without subscription", premToggle));

    auto* noAdsToggle = new ToggleSwitch;
    noAdsToggle->setChecked(prem->noAdsEnabled());
    connect(noAdsToggle, &ToggleSwitch::toggled, prem, &LocalPremium::setNoAds);
    lay->addWidget(makeRow("Remove Ads",
        "Hide all sponsored messages", noAdsToggle));

    lay->addWidget(makeSectionHeader("Visual Phone Number"));

    auto* phoneEdit = new QLineEdit(prem->customPhone());
    phoneEdit->setPlaceholderText("+1 (234) 567-8900");
    connect(phoneEdit, &QLineEdit::editingFinished, [prem, phoneEdit]{
        prem->setCustomPhone(phoneEdit->text().trimmed());
    });
    lay->addWidget(makeRow("Custom Phone",
        "Displayed locally in your profile only", phoneEdit));

    auto* clearPhoneBtn = new QPushButton("Clear");
    clearPhoneBtn->setObjectName("dangerBtn");
    connect(clearPhoneBtn, &QPushButton::clicked, [prem, phoneEdit]{
        prem->clearCustomPhone();
        phoneEdit->clear();
    });
    lay->addWidget(makeRow("", "", clearPhoneBtn));

    lay->addWidget(makeSectionHeader("FainsGram Badge"));
    auto* badge = FG().badge();

    auto* badgeToggle = new ToggleSwitch;
    badgeToggle->setChecked(badge->badgesEnabled());
    connect(badgeToggle, &ToggleSwitch::toggled, badge, &CustomBadge::setBadgesEnabled);
    lay->addWidget(makeRow("Show ⚡ Badge",
        "Show badge next to FainsGram users", badgeToggle));

    auto* selfMarkToggle = new ToggleSwitch;
    selfMarkToggle->setChecked(badge->selfMarkEnabled());
    connect(selfMarkToggle, &ToggleSwitch::toggled, badge, &CustomBadge::setSelfMarkEnabled);
    lay->addWidget(makeRow("Mark Myself",
        "Add #FG to your bio so others see your badge", selfMarkToggle));

    lay->addStretch();
    scroll->setWidget(content);
    scroll->setWidgetResizable(true);
    return scroll;
}

// ── About page ────────────────────────────────────────────────────────────────

QWidget* FainsGramSettingsPanel::buildAboutPage() {
    auto* content = new QWidget;
    auto* lay = new QVBoxLayout(content);
    lay->setContentsMargins(20, 20, 20, 20);
    lay->setSpacing(16);

    auto* logo = new QLabel("⚡");
    logo->setAlignment(Qt::AlignCenter);
    logo->setStyleSheet("font-size: 64px;");

    auto* title = new QLabel("FainsGram");
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 28px; font-weight: bold; color: #ff6b35;");

    auto* ver = new QLabel(QString("Version %1")
                           .arg(FainsGramController::kForkVersion));
    ver->setAlignment(Qt::AlignCenter);
    ver->setStyleSheet("font-size: 14px; color: #707890;");

    auto* desc = new QLabel(
        "FainsGram is a custom Telegram Desktop fork\n"
        "with privacy, customization and power-user\n"
        "features built in.\n\n"
        "Built on top of official TDesktop source.\n"
        "All features are client-side only.");
    desc->setAlignment(Qt::AlignCenter);
    desc->setStyleSheet("font-size: 13px; color: #a0a8c0; line-height: 1.6;");
    desc->setWordWrap(true);

    auto* sep = makeSeparator();

    auto* features = new QLabel(
        "👻  Ghost Mode  •  🗄 Message Vault\n"
        "👤  Unlimited Accounts  •  🎨 Theme Engine\n"
        "⭐  Local Premium  •  ⚡ FainsGram Badge");
    features->setAlignment(Qt::AlignCenter);
    features->setStyleSheet("font-size: 12px; color: #707890; line-height: 1.8;");

    lay->addStretch();
    lay->addWidget(logo);
    lay->addWidget(title);
    lay->addWidget(ver);
    lay->addSpacing(12);
    lay->addWidget(desc);
    lay->addWidget(sep);
    lay->addWidget(features);
    lay->addStretch();

    return content;
}

void FainsGramSettingsPanel::refresh() {
    // Re-read from controller when panel is shown
}

} // namespace FainsGram

namespace Settings {

FainsGramSection::FainsGramSection(
    QWidget *parent,
    not_null<Window::SessionController*> controller)
: Section(parent) {
    setupContent(controller);
}

rpl::producer<QString> FainsGramSection::title() {
    return rpl::single(QString("FainsGram"));
}

void FainsGramSection::setupContent(not_null<Window::SessionController*> controller) {
    const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);

    // SubHeader Description (matching AyuGram Screenshot 2)
    Ui::AddSubsectionTitle(content, rpl::single(QString("FainsGram Desktop v1.0.0")));

    auto desc = content->add(object_ptr<Ui::FlatLabel>(
        content,
        u"Telegram Desktop fork focused on customization and ToS-breaking features."_q,
        st::defaultFlatLabel
    ));
    Ui::AddDivider(content);
    Ui::AddSkip(content);

    // Categories (Subsection Title matching Screenshot 2)
    Ui::AddSubsectionTitle(content, rpl::single(QString("Categories")));

    // FainsGram / Ghost Mode row
    AddButtonWithIcon(
        content,
        rpl::single(QString("FainsGram (Ghost Mode)")),
        st::settingsButton
    )->setClickedCallback([=] {
        controller->show(Box([=](not_null<Ui::GenericBox*> ghostBox) {
            ghostBox->setTitle(rpl::single(QString("Ghost Mode")));
            ghostBox->addButton(tr::lng_close(), [=] { ghostBox->closeBox(); });
            auto g = ghostBox->verticalLayout();

            Ui::AddSubsectionTitle(g, rpl::single(QString("GHOST MODE")));

            auto ghostChat = g->add(object_ptr<Button>(
                g,
                rpl::single(QString("No Read Receipts (Нечиталка сообщений)")),
                st::settingsButtonNoIcon
            ))->toggleOn(rpl::single(FainsGram::FainsGramController::instance().ghost()->noReadReceipts()));
            ghostChat->toggledValue() | rpl::start_with_next([=](bool on) {
                FainsGram::FainsGramController::instance().ghost()->setNoReadReceipts(on);
            }, ghostBox->lifetime());

            auto ghostStories = g->add(object_ptr<Button>(
                g,
                rpl::single(QString("Ghost Stories (Нечиталка сторис)")),
                st::settingsButtonNoIcon
            ))->toggleOn(rpl::single(FainsGram::FainsGramController::instance().ghost()->ghostStories()));
            ghostStories->toggledValue() | rpl::start_with_next([=](bool on) {
                FainsGram::FainsGramController::instance().ghost()->setGhostStories(on);
            }, ghostBox->lifetime());

            auto ghostOnline = g->add(object_ptr<Button>(
                g,
                rpl::single(QString("Hide Online Status (Скрыть онлайн)")),
                st::settingsButtonNoIcon
            ))->toggleOn(rpl::single(FainsGram::FainsGramController::instance().ghost()->hideOnline()));
            ghostOnline->toggledValue() | rpl::start_with_next([=](bool on) {
                FainsGram::FainsGramController::instance().ghost()->setHideOnline(on);
            }, ghostBox->lifetime());

            auto ghostTyping = g->add(object_ptr<Button>(
                g,
                rpl::single(QString("No Typing Indicator (Скрыть набор)")),
                st::settingsButtonNoIcon
            ))->toggleOn(rpl::single(FainsGram::FainsGramController::instance().ghost()->noTypingIndicator()));
            ghostTyping->toggledValue() | rpl::start_with_next([=](bool on) {
                FainsGram::FainsGramController::instance().ghost()->setNoTypingIndicator(on);
            }, ghostBox->lifetime());
        }));
    });

    // Message Vault row
    AddButtonWithIcon(
        content,
        rpl::single(QString("Message Vault (Удалённые сообщения)")),
        st::settingsButton
    )->setClickedCallback([=] {
        controller->show(Box([=](not_null<Ui::GenericBox*> vaultBox) {
            vaultBox->setTitle(rpl::single(QString("Message Vault")));
            vaultBox->addButton(tr::lng_close(), [=] { vaultBox->closeBox(); });
            auto v = vaultBox->verticalLayout();

            Ui::AddSubsectionTitle(v, rpl::single(QString("MESSAGE VAULT")));

            v->add(object_ptr<Button>(
                v,
                rpl::single(QString("Сохранение удалённых сообщений: Включено")),
                st::settingsButtonNoIcon
            ));
        }));
    });

    // Local Premium row
    AddButtonWithIcon(
        content,
        rpl::single(QString("Local Premium (Бесплатный Премиум)")),
        st::settingsButton
    );

    // Appearance row
    AddButtonWithIcon(
        content,
        rpl::single(QString("Appearance (Кастомизация)")),
        st::settingsButton
    );

    // Unlimited Accounts row
    AddButtonWithIcon(
        content,
        rpl::single(QString("Unlimited Accounts (Безлимит аккаунтов)")),
        st::settingsButton
    );

    // Other row
    AddButtonWithIcon(
        content,
        rpl::single(QString("Other (Дополнительно)")),
        st::settingsButton
    );

    Ui::AddDivider(content);
    Ui::AddSkip(content);

    // Links
    Ui::AddSubsectionTitle(content, rpl::single(QString("Links")));

    AddButtonWithIcon(
        content,
        rpl::single(QString("Channel                                                   @fainsgram")),
        st::settingsButton
    );

    AddButtonWithIcon(
        content,
        rpl::single(QString("Chats                                                        @fainsgramchat")),
        st::settingsButton
    );

    AddButtonWithIcon(
        content,
        rpl::single(QString("Documentation                                      fainsgram.app")),
        st::settingsButton
    );

    Ui::ResizeFitChild(this, content);
}

} // namespace Settings

namespace FainsGram {

void ShowFainsGramCategoryBox(not_null<Window::SessionController*> controller) {
    controller->show(Box([=](not_null<Ui::GenericBox*> box) {
        box->setStyle(st::boostBox);
        box->setWidth(540);
        box->setTitle(rpl::single(QString("FainsGram")));
        box->addButton(tr::lng_close(), [=] { box->closeBox(); });
        box->setCloseByEscape(true);
        box->setCloseByOutsideClick(true);

        auto container = box->verticalLayout();

        auto master = container->add(object_ptr<Settings::Button>(
            container, rpl::single(QString("Master Ghost Mode")), st::settingsButtonNoIcon
        ))->toggleOn(rpl::single(FainsGramController::instance().ghost()->isEnabled()));
        master->toggledValue() | rpl::start_with_next([=](bool on) { FainsGramController::instance().ghost()->setEnabled(on); }, box->lifetime());

        auto read = container->add(object_ptr<Settings::Button>(
            container, rpl::single(QString("No Read Receipts")), st::settingsButtonNoIcon
        ))->toggleOn(rpl::single(FainsGramController::instance().ghost()->noReadReceipts()));
        read->toggledValue() | rpl::start_with_next([=](bool on) { FainsGramController::instance().ghost()->setNoReadReceipts(on); }, box->lifetime());

        auto online = container->add(object_ptr<Settings::Button>(
            container, rpl::single(QString("Hide Online Status")), st::settingsButtonNoIcon
        ))->toggleOn(rpl::single(FainsGramController::instance().ghost()->hideOnline()));
        online->toggledValue() | rpl::start_with_next([=](bool on) { FainsGramController::instance().ghost()->setHideOnline(on); }, box->lifetime());

        auto stories = container->add(object_ptr<Settings::Button>(
            container, rpl::single(QString("Ghost Stories")), st::settingsButtonNoIcon
        ))->toggleOn(rpl::single(FainsGramController::instance().ghost()->ghostStories()));
        stories->toggledValue() | rpl::start_with_next([=](bool on) { FainsGramController::instance().ghost()->setGhostStories(on); }, box->lifetime());

        auto typing = container->add(object_ptr<Settings::Button>(
            container, rpl::single(QString("No Typing Indicator")), st::settingsButtonNoIcon
        ))->toggleOn(rpl::single(FainsGramController::instance().ghost()->noTypingIndicator()));
        typing->toggledValue() | rpl::start_with_next([=](bool on) { FainsGramController::instance().ghost()->setNoTypingIndicator(on); }, box->lifetime());
    }));
}

void ShowFiltersCategoryBox(not_null<Window::SessionController*> controller) {
    controller->show(Box([=](not_null<Ui::GenericBox*> box) {
        box->setStyle(st::boostBox);
        box->setWidth(540);
        box->setTitle(rpl::single(QString("Vault & History")));
        box->addButton(tr::lng_close(), [=] { box->closeBox(); });
        box->setCloseByEscape(true);
        box->setCloseByOutsideClick(true);

        auto container = box->verticalLayout();

        container->add(object_ptr<Settings::Button>(
            container, rpl::single(QString("Save Deleted Messages")), st::settingsButtonNoIcon
        ))->toggleOn(rpl::single(true));

        container->add(object_ptr<Settings::Button>(
            container, rpl::single(QString("Save Disappearing / One-View Media")), st::settingsButtonNoIcon
        ))->toggleOn(rpl::single(true));

        container->add(object_ptr<Settings::Button>(
            container, rpl::single(QString("Save Edited Message History")), st::settingsButtonNoIcon
        ))->toggleOn(rpl::single(true));

        container->add(object_ptr<Settings::Button>(
            container, rpl::single(QString("Permanent Stories Vault")), st::settingsButtonNoIcon
        ))->toggleOn(rpl::single(true));
    }));
}

void ShowGeneralCategoryBox(not_null<Window::SessionController*> controller) {
    controller->show(Box([=](not_null<Ui::GenericBox*> box) {
        box->setStyle(st::boostBox);
        box->setWidth(540);
        box->setTitle(rpl::single(QString("General")));
        box->addButton(tr::lng_close(), [=] { box->closeBox(); });
        box->setCloseByEscape(true);
        box->setCloseByOutsideClick(true);

        auto container = box->verticalLayout();

        auto unlim = container->add(object_ptr<Settings::Button>(
            container, rpl::single(QString("Unlimited Accounts")), st::settingsButtonNoIcon
        ))->toggleOn(rpl::single(FainsGramController::instance().accounts()->isUnlimitedEnabled()));
        unlim->toggledValue() | rpl::start_with_next([=](bool on) { FainsGramController::instance().accounts()->setUnlimitedEnabled(on); }, box->lifetime());

        auto ads = container->add(object_ptr<Settings::Button>(
            container, rpl::single(QString("Hide Ads")), st::settingsButtonNoIcon
        ))->toggleOn(rpl::single(FainsGramController::instance().premium()->noAdsEnabled()));
        ads->toggledValue() | rpl::start_with_next([=](bool on) { FainsGramController::instance().premium()->setNoAds(on); }, box->lifetime());
    }));
}

void ShowAppearanceCategoryBox(not_null<Window::SessionController*> controller) {
    controller->show(Box([=](not_null<Ui::GenericBox*> box) {
        box->setStyle(st::boostBox);
        box->setWidth(540);
        box->setTitle(rpl::single(QString("Appearance")));
        box->addButton(tr::lng_close(), [=] { box->closeBox(); });
        box->setCloseByEscape(true);
        box->setCloseByOutsideClick(true);

        auto container = box->verticalLayout();
        auto* theme = FainsGramController::instance().theme();

        // --- Profile ---
        Ui::AddSubsectionTitle(container, rpl::single(QString("Profile")));

        // Custom Phone
        auto phoneInput = container->add(
            object_ptr<Ui::InputField>(
                container, st::defaultInputField,
                rpl::single(QString("Custom phone number")),
                FainsGramController::instance().settings().value("CustomProfile/phone", "").toString()
            ),
            st::boxRowPadding
        );
        Ui::AddSkip(container);

        // Custom Username
        auto usernameInput = container->add(
            object_ptr<Ui::InputField>(
                container, st::defaultInputField,
                rpl::single(QString("Custom username  (e.g. @durov)")),
                FainsGramController::instance().settings().value("CustomProfile/username", "").toString()
            ),
            st::boxRowPadding
        );
        Ui::AddSkip(container);

        auto saveProfileBtn = container->add(object_ptr<Settings::Button>(
            container, rpl::single(QString("Apply Profile Changes")), st::settingsButtonNoIcon
        ));
        saveProfileBtn->setClickedCallback([=] {
            FainsGramController::instance().settings().setValue(
                "CustomProfile/phone", phoneInput->getLastText().trimmed());
            FainsGramController::instance().settings().setValue(
                "CustomProfile/username", usernameInput->getLastText().trimmed());
            FainsGramController::instance().premium()->apply();
            if (Core::App().someSessionExists()) {
                controller->session().changes().peerUpdated(
                    controller->session().user(),
                    Data::PeerUpdate::Flag::PhoneNumber
                    | Data::PeerUpdate::Flag::Username
                    | Data::PeerUpdate::Flag::Usernames
                    | Data::PeerUpdate::Flag::Name
                    | Data::PeerUpdate::Flag::FullInfo
                );
            }
        });

        Ui::AddSkip(container);
    }));
}


void ShowChatsCategoryBox(not_null<Window::SessionController*> controller) {
    controller->show(Box([=](not_null<Ui::GenericBox*> box) {
        box->setStyle(st::boostBox);
        box->setWidth(540);
        box->setTitle(rpl::single(QString("Chats")));
        box->addButton(tr::lng_close(), [=] { box->closeBox(); });
        box->setCloseByEscape(true);
        box->setCloseByOutsideClick(true);

        auto container = box->verticalLayout();

        container->add(object_ptr<Settings::Button>(
            container, rpl::single(QString("Show Deleted Badge")), st::settingsButtonNoIcon
        ))->toggleOn(rpl::single(true));

        container->add(object_ptr<Settings::Button>(
            container, rpl::single(QString("Show Edited History")), st::settingsButtonNoIcon
        ))->toggleOn(rpl::single(true));
    }));
}

void ShowOtherCategoryBox(not_null<Window::SessionController*> controller) {
    controller->show(Box([=](not_null<Ui::GenericBox*> box) {
        box->setStyle(st::boostBox);
        box->setWidth(540);
        box->setTitle(rpl::single(QString("Other")));
        box->addButton(tr::lng_close(), [=] { box->closeBox(); });
        box->setCloseByEscape(true);
        box->setCloseByOutsideClick(true);

        auto container = box->verticalLayout();

        container->add(object_ptr<Settings::Button>(
            container, rpl::single(QString("Local Storage Encryption")), st::settingsButtonNoIcon
        ))->toggleOn(rpl::single(true));
    }));
}

void ShowProxyCategoryBox(not_null<Window::SessionController*> controller) {
    controller->show(Box([=](not_null<Ui::GenericBox*> box) {
        box->setStyle(st::boostBox);
        box->setWidth(540);
        box->setTitle(rpl::single(QString("Proxy (TG WS Proxy)")));
        box->addButton(tr::lng_close(), [=] { box->closeBox(); });
        box->setCloseByEscape(true);
        box->setCloseByOutsideClick(true);

        auto container = box->verticalLayout();
        auto* ws = FainsGramController::instance().wsproxy();

        // ── Master enable/disable toggle ────────────────────────────────────────
        auto enable = container->add(object_ptr<Settings::Button>(
            container, rpl::single(QString("Enable local MTProto proxy")), st::settingsButtonNoIcon
        ))->toggleOn(rpl::single(ws->isEnabled()));
        enable->toggledValue() | rpl::start_with_next([=](bool on) {
            ws->setEnabled(on);
        }, box->lifetime());

        Ui::AddDivider(container);
        Ui::AddSkip(container);

        // ── Live status ─────────────────────────────────────────────────────────
        Ui::AddSubsectionTitle(container, rpl::single(QString("Status")));
        auto* statusLabel = container->add(
            object_ptr<Ui::FlatLabel>(container, ws->statusText(), st::defaultFlatLabel),
            style::margins(22, 0, 22, 8));
        QObject::connect(ws, &WsProxyController::statusChanged, statusLabel,
            [statusLabel](const QString& t) { statusLabel->setText(t); });

        Ui::AddSkip(container);

        // ── Port row ────────────────────────────────────────────────────────────
        Ui::AddSubsectionTitle(container, rpl::single(QString("Connection")));
        {
            auto* row = container->add(object_ptr<Ui::RpWidget>(container),
                style::margins(22, 4, 22, 4));
            auto* hl = new QHBoxLayout(row);
            hl->setContentsMargins(0, 0, 0, 0);
            auto* lbl = new QLabel("Port", row);
            lbl->setMinimumWidth(90);
            hl->addWidget(lbl);

            auto* port = new QSpinBox(row);
            port->setRange(1, 65535);
            port->setValue(ws->port());
            QObject::connect(port, QOverload<int>::of(&QSpinBox::valueChanged),
                [=](int v) { ws->setPort(v); });
            hl->addWidget(port);
            hl->addStretch(1);
        }

        // ── Secret (MTProto dd key) row ─────────────────────────────────────────
        {
            auto* row = container->add(object_ptr<Ui::RpWidget>(container),
                style::margins(22, 4, 22, 4));
            auto* hl = new QHBoxLayout(row);
            hl->setContentsMargins(0, 0, 0, 0);
            auto* lbl = new QLabel("Secret (dd key)", row);
            lbl->setMinimumWidth(90);
            hl->addWidget(lbl);

            auto* secret = new QLineEdit(row);
            secret->setText(ws->secret());
            secret->setMinimumWidth(180);
            QObject::connect(secret, &QLineEdit::editingFinished, [=] {
                ws->setSecret(secret->text());
            });
            hl->addWidget(secret);

            auto* regen = new QPushButton("↻", row);
            regen->setToolTip("Regenerate secret");
            QObject::connect(regen, &QPushButton::clicked, [=] {
                QByteArray raw(16, 0);
                auto* rnd = QRandomGenerator::global();
                for (int i = 0; i < 16; ++i) raw[i] = uint8_t(rnd->generate() & 0xFF);
                const QString sec = QString::fromUtf8(raw.toHex());
                ws->setSecret(sec);
                secret->setText(sec);
            });
            hl->addWidget(regen);

            auto* copy = new QPushButton("Copy", row);
            QObject::connect(copy, &QPushButton::clicked, [=] {
                QApplication::clipboard()->setText("dd" + ws->secret());
            });
            hl->addWidget(copy);
        }

        Ui::AddSkip(container);
        Ui::AddDivider(container);
        Ui::AddSkip(container);

        // ── Hint ─────────────────────────────────────────────────────────────────
        container->add(
            object_ptr<Ui::FlatLabel>(
                container,
                rpl::single(QString(
                    "When enabled, FainsGram starts a local proxy on 127.0.0.1 and "
                    "automatically routes Telegram through it (MTProto secret \"dd\" + the key above). "
                    "Disable to revert to the system connection.")),
                st::defaultFlatLabel),
            style::margins(22, 4, 22, 8));
    }));
}

void ShowSettingsBox(not_null<Window::SessionController*> controller) {
    controller->show(Box([=](not_null<Ui::GenericBox*> box) {
        box->setStyle(st::boostBox);
        box->setWidth(500);
        box->setTitle(rpl::single(QString("FainsGram")));
        box->addButton(tr::lng_close(), [=] { box->closeBox(); });
        box->setCloseByEscape(true);
        box->setCloseByOutsideClick(true);

        auto container = box->verticalLayout();

        // ── Header: icon + versions + channel ─────────────────────────────
        {
            // App icon (small) + name + versions
            auto* hdr = container->add(
                object_ptr<Ui::FlatLabel>(
                    container,
                    u"FainsGram"_q,
                    st::defaultFlatLabel
                ),
                style::margins(22, 8, 22, 0)
            );

            container->add(
                object_ptr<Ui::FlatLabel>(
                    container,
                    rpl::single(QString("Fork v%1  \u2022  Telegram v%2")
                        .arg(FainsGramController::kForkVersion)
                        .arg(QString::fromLatin1(AppVersionStr))),
                    st::defaultFlatLabel
                ),
                style::margins(22, 2, 22, 0)
            );

            // Channel link button
            auto* chBtn = container->add(object_ptr<Settings::Button>(
                container,
                rpl::single(QString("Channel  @FainsGram")),
                st::settingsButtonNoIcon
            ));
            chBtn->setClickedCallback([=] {
                QDesktopServices::openUrl(
                    QUrl(u"https://t.me/FainsGram"_q));
            });
        }

        Ui::AddDivider(container);
        Ui::AddSkip(container);
        Ui::AddSubsectionTitle(container, rpl::single(QString("Categories")));

        // 1. FainsGram (Ghost + Vault)
        Settings::AddButtonWithIcon(
            container, rpl::single(QString("FainsGram")), st::settingsButtonNoIcon
        )->setClickedCallback([=] { ShowFainsGramCategoryBox(controller); });

        // 2. Filters
        Settings::AddButtonWithIcon(
            container, rpl::single(QString("Filters")), st::settingsButtonNoIcon
        )->setClickedCallback([=] { ShowFiltersCategoryBox(controller); });

        // 3. General
        Settings::AddButtonWithIcon(
            container, rpl::single(QString("General")), st::settingsButtonNoIcon
        )->setClickedCallback([=] { ShowGeneralCategoryBox(controller); });

        // 4. Appearance
        Settings::AddButtonWithIcon(
            container, rpl::single(QString("Appearance")), st::settingsButtonNoIcon
        )->setClickedCallback([=] { ShowAppearanceCategoryBox(controller); });

        // 5. Chats
        Settings::AddButtonWithIcon(
            container, rpl::single(QString("Chats")), st::settingsButtonNoIcon
        )->setClickedCallback([=] { ShowChatsCategoryBox(controller); });

        // 6. Proxy (TG WS Proxy)
        Settings::AddButtonWithIcon(
            container, rpl::single(QString("Proxy (TG WS Proxy)")), st::settingsButtonNoIcon
        )->setClickedCallback([=] { ShowProxyCategoryBox(controller); });

        Ui::AddDivider(container);
        Ui::AddSkip(container);
        Ui::AddSubsectionTitle(container, rpl::single(QString("Donate for coffee plz:")));

        container->add(
            object_ptr<Ui::FlatLabel>(
                container,
                u"UQATWyIGRvaWYiwFuc4F2a_TE8YrVTa9tdJVleD7qXjrsgFD - ton"_q,
                st::defaultFlatLabel
            ),
            style::margins(22, 6, 22, 4)
        );

        container->add(
            object_ptr<Ui::FlatLabel>(
                container,
                u"TJiUosMNuA5D4xaD2Q96J4hDdNiXdzu2ZL - usdt trc 20"_q,
                st::defaultFlatLabel
            ),
            style::margins(22, 4, 22, 12)
        );

        Ui::AddSkip(container);
    }));
}

} // namespace FainsGram
