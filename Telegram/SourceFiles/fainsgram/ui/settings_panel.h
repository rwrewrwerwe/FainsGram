/*
 * FainsGram — Settings Panel UI
 * ================================
 * Rendered as a new section ABOVE "My Account" in the sidebar.
 * Fully custom Qt widget with:
 *   - Animated toggle switches
 *   - Section tabs: Ghost | Vault | Accounts | Theme | Premium | About
 *   - Glassmorphism header card showing fork name + badge
 *
 * Copyright (c) 2024 FainsGram. MIT License.
 */

#pragma once

#include <QtWidgets/QWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QSlider>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtCore/QPropertyAnimation>
#include "settings/settings_common_session.h"
#include "ui/rp_widget.h"

namespace Window {
class SessionController;
}

namespace FainsGram {

// ── Animated toggle switch (iOS-style) ───────────────────────────────────────
class ToggleSwitch final : public QWidget {
    Q_OBJECT
    Q_PROPERTY(int offset READ offset WRITE setOffset)
public:
    explicit ToggleSwitch(QWidget* parent = nullptr);

    bool    isChecked() const { return _checked; }
    int     offset()    const { return _offset; }
    void    setOffset(int o)  { _offset = o; update(); }

public Q_SLOTS:
    void setChecked(bool on);
    void toggle() { setChecked(!_checked); }

Q_SIGNALS:
    void toggled(bool on);

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    QSize sizeHint() const override { return {52, 28}; }

private:
    bool                _checked = false;
    int                 _offset  = 4;
    QPropertyAnimation* _anim;
};

// ── Section button (tab bar item) ────────────────────────────────────────────
class SectionButton final : public QPushButton {
    Q_OBJECT
public:
    explicit SectionButton(const QString& icon,
                           const QString& label,
                           QWidget* parent = nullptr);
    void setActive(bool on);
};

// ── Main settings panel ──────────────────────────────────────────────────────
class FainsGramSettingsPanel final : public Ui::RpWidget {
    Q_OBJECT
public:
    explicit FainsGramSettingsPanel(QWidget* parent = nullptr);

    // Called when the panel is shown
    void refresh();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void buildHeader();
    void buildTabBar();
    void buildPages();

    // Individual pages
    QWidget* buildGhostPage();
    QWidget* buildVaultPage();
    QWidget* buildAccountsPage();
    QWidget* buildThemePage();
    QWidget* buildPremiumPage();
    QWidget* buildAboutPage();

    // Helper to add a settings row (label + toggle/widget)
    QWidget* makeRow(const QString& title, const QString& subtitle, QWidget* control);
    QWidget* makeSeparator();
    QWidget* makeSectionHeader(const QString& text);

    void switchToPage(int idx);
    void applyStyle();

    QVBoxLayout*    _mainLayout = nullptr;
    QWidget*        _header     = nullptr;
    QHBoxLayout*    _tabBar     = nullptr;
    QStackedWidget* _pages      = nullptr;

    QVector<SectionButton*> _tabs;
    int _currentPage = 0;
};

void ShowSettingsBox(not_null<Window::SessionController*> controller);

} // namespace FainsGram

namespace Settings {

class FainsGramSection : public Section<FainsGramSection> {
public:
    FainsGramSection(
        QWidget *parent,
        not_null<Window::SessionController*> controller);

    [[nodiscard]] rpl::producer<QString> title() override;

private:
    void setupContent(not_null<Window::SessionController*> controller);
};

} // namespace Settings
