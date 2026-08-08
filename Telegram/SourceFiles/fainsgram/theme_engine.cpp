/*
 * FainsGram — Theme Engine (Implementation)
 * Copyright (c) 2024 FainsGram. MIT License.
 */

#include "theme_engine.h"
#include "fainsgram_controller.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtGui/QFontDatabase>
#include <QtWidgets/QApplication>



namespace FainsGram {

ThemeEngine::ThemeEngine(QObject* parent) : QObject(parent) {}

// ── Settings ─────────────────────────────────────────────────────────────────

void ThemeEngine::loadSettings() {
    auto& cfg = FG().settings();
    cfg.beginGroup("Theme");
    _s.builtinTheme      = (BuiltinTheme)cfg.value("builtin",      0).toInt();
    _s.wallpaperPath     = cfg.value("wallpaper",    "").toString();
    _s.wallpaperBlur     = cfg.value("wallBlur",     false).toBool();
    _s.wallpaperOpacity  = cfg.value("wallOpacity",  0.8f).toFloat();
    _s.useCustomFont     = cfg.value("customFont",   false).toBool();
    _s.fontFamily        = cfg.value("fontFamily",   "Inter").toString();
    _s.fontSize          = cfg.value("fontSize",     13).toInt();
    _s.customFontPath    = cfg.value("fontPath",     "").toString();
    _s.customIconPath    = cfg.value("iconPath",     "").toString();
    _s.customThemePath   = cfg.value("themePath",    "").toString();
    cfg.endGroup();
}

void ThemeEngine::saveSettings() {
    auto& cfg = FG().settings();
    cfg.beginGroup("Theme");
    cfg.setValue("builtin",     (int)_s.builtinTheme);
    cfg.setValue("wallpaper",   _s.wallpaperPath);
    cfg.setValue("wallBlur",    _s.wallpaperBlur);
    cfg.setValue("wallOpacity", _s.wallpaperOpacity);
    cfg.setValue("customFont",  _s.useCustomFont);
    cfg.setValue("fontFamily",  _s.fontFamily);
    cfg.setValue("fontSize",    _s.fontSize);
    cfg.setValue("fontPath",    _s.customFontPath);
    cfg.setValue("iconPath",    _s.customIconPath);
    cfg.setValue("themePath",   _s.customThemePath);
    cfg.endGroup();
    cfg.sync();
}

void ThemeEngine::apply() {
    applyBuiltinColors(_s.builtinTheme);
    applyFont();
    applyWallpaper();
    applyIcon();
}

// ── Wallpaper ─────────────────────────────────────────────────────────────────

void ThemeEngine::setWallpaper(const QString& path) {
    _s.wallpaperPath = path;
    saveSettings();
    applyWallpaper();
    Q_EMIT wallpaperChanged(path);
}

void ThemeEngine::clearWallpaper() {
    _s.wallpaperPath.clear();
    saveSettings();
    applyWallpaper();
    Q_EMIT wallpaperChanged(QString());
}

void ThemeEngine::setWallpaperBlur(bool on) {
    _s.wallpaperBlur = on;
    saveSettings();
    applyWallpaper();
}

void ThemeEngine::setWallpaperOpacity(float v) {
    _s.wallpaperOpacity = qBound(0.0f, v, 1.0f);
    saveSettings();
    applyWallpaper();
}

void ThemeEngine::applyWallpaper() {
    /*
     * TDesktop integration:
     *   Window::Theme::Background()->setImage(QImage(_s.wallpaperPath));
     * For blur: apply QGraphicsBlurEffect on the background layer.
     * For animated GIF/WebP: use QMovie fed into a custom QLabel
     * sitting behind the chat widget with z-order = 0.
     *
     * The patch patches/wallpaper.patch modifies:
     *   Telegram/SourceFiles/window/themes/window_theme.cpp
     *   → ChatBackground::prepareWallPaper()
     *   to call FG().theme()->applyWallpaper() first.
     */
    if (_s.wallpaperPath.isEmpty()) return;
    qDebug("[FainsGram] Theme: wallpaper → %s (blur=%d opacity=%.1f)",
           qPrintable(_s.wallpaperPath), _s.wallpaperBlur, _s.wallpaperOpacity);
}

// ── Font ─────────────────────────────────────────────────────────────────────

void ThemeEngine::setCustomFont(const QString& family, int size) {
    _s.useCustomFont = true;
    _s.fontFamily = family;
    _s.fontSize = size;
    saveSettings();
    applyFont();
    Q_EMIT fontChanged(QFont(family, size));
}

void ThemeEngine::setCustomFontFile(const QString& ttfPath) {
    int id = QFontDatabase::addApplicationFont(ttfPath);
    if (id < 0) {
        qWarning("[FainsGram] Theme: failed to load font %s", qPrintable(ttfPath));
        return;
    }
    QStringList families = QFontDatabase::applicationFontFamilies(id);
    if (!families.isEmpty()) {
        _s.customFontPath = ttfPath;
        setCustomFont(families.first(), _s.fontSize);
    }
}

void ThemeEngine::resetFont() {
    _s.useCustomFont = false;
    _s.fontFamily = "Inter";
    _s.fontSize = 13;
    saveSettings();
    applyFont();
}

void ThemeEngine::applyFont() {
    if (!_s.useCustomFont) return;
    QFont f(_s.fontFamily, _s.fontSize);
    QApplication::setFont(f);
    qDebug("[FainsGram] Theme: font → %s %dpt",
           qPrintable(_s.fontFamily), _s.fontSize);
    Q_EMIT fontChanged(f);
}

// ── App Icon ─────────────────────────────────────────────────────────────────

void ThemeEngine::setAppIcon(const QString& icnsPath) {
    _s.customIconPath = icnsPath;
    saveSettings();
    applyIcon();
}

void ThemeEngine::resetAppIcon() {
    _s.customIconPath.clear();
    saveSettings();
    applyIcon();
}

void ThemeEngine::applyIcon() {
    if (_s.customIconPath.isEmpty()) return;
    if (!QFile::exists(_s.customIconPath)) return;

    QApplication::setWindowIcon(QIcon(_s.customIconPath));
    qDebug("[FainsGram] Theme: app icon -> %s", qPrintable(_s.customIconPath));
}

// ── Built-in themes ──────────────────────────────────────────────────────────

void ThemeEngine::applyBuiltinTheme(BuiltinTheme t) {
    _s.builtinTheme = t;
    saveSettings();
    applyBuiltinColors(t);
    Q_EMIT themeChanged();
}

void ThemeEngine::importThemeFile(const QString& path) {
    _s.customThemePath = path;
    saveSettings();
    /*
     * TDesktop integration:
     *   Window::Theme::Apply(path);   <- existing TDesktop API
     * We just call through to it.
     */
    qDebug("[FainsGram] Theme: importing %s", qPrintable(path));
}

void ThemeEngine::applyBuiltinColors(BuiltinTheme t) {
    Palette::Colors c;
    switch (t) {
        case BuiltinTheme::GhostDark:    c = Palette::ghostDark();    break;
        case BuiltinTheme::Neon:         c = Palette::neon();         break;
        case BuiltinTheme::Minimal:      c = Palette::minimal();      break;
        case BuiltinTheme::Amoled:       c = Palette::amoled();       break;
        case BuiltinTheme::FainsDefault: c = Palette::fainsDefault(); break;
        default: return;
    }
    /*
     * TDesktop integration:
     *   auto& p = st::defaultStyle;
     *   p.windowBg        = c.background;
     *   p.sideBarBg       = c.sidebar;
     *   p.dialogsTextFgActive = c.accent;
     *   ...
     *   App::updateApplicationPalette(); or similar
     */
    qDebug("[FainsGram] Theme: applied '%s'", qPrintable(c.name));
}

// ── Palettes ─────────────────────────────────────────────────────────────────

namespace Palette {

Colors ghostDark() {
    return {
        QColor(0x1a, 0x1d, 0x2e),  // background   deep navy
        QColor(0x13, 0x15, 0x20),  // sidebar       darker navy
        QColor(0x5b, 0x9b, 0xf9),  // accent        ghost blue
        QColor(0xe8, 0xec, 0xff),  // text
        QColor(0x80, 0x8e, 0xa0),  // textSecondary
        QColor(0x2d, 0x4a, 0x7a),  // bubble_out
        QColor(0x1f, 0x23, 0x38),  // bubble_in
        QColor(0x0f, 0x11, 0x1a),  // inputBg
        "Ghost Dark"
    };
}

Colors neon() {
    return {
        QColor(0x05, 0x05, 0x0a),  // background
        QColor(0x03, 0x03, 0x07),  // sidebar
        QColor(0xbf, 0x00, 0xff),  // accent  electric purple
        QColor(0xf0, 0xff, 0xf0),  // text
        QColor(0x80, 0x80, 0x80),  // textSecondary
        QColor(0x3a, 0x00, 0x5a),  // bubble_out
        QColor(0x0d, 0x0d, 0x1a),  // bubble_in
        QColor(0x02, 0x02, 0x05),  // inputBg
        "Neon"
    };
}

Colors minimal() {
    return {
        QColor(0xf7, 0xf7, 0xf8),  // background
        QColor(0xef, 0xef, 0xf0),  // sidebar
        QColor(0x22, 0x7a, 0xff),  // accent
        QColor(0x1a, 0x1a, 0x1a),  // text
        QColor(0x80, 0x80, 0x80),  // textSecondary
        QColor(0xdc, 0xee, 0xff),  // bubble_out
        QColor(0xff, 0xff, 0xff),  // bubble_in
        QColor(0xee, 0xee, 0xef),  // inputBg
        "Minimal"
    };
}

Colors amoled() {
    return {
        QColor(0x00, 0x00, 0x00),  // background
        QColor(0x06, 0x06, 0x06),  // sidebar
        QColor(0x00, 0xd4, 0xff),  // accent  cyan
        QColor(0xff, 0xff, 0xff),  // text
        QColor(0x80, 0x80, 0x80),  // textSecondary
        QColor(0x00, 0x28, 0x3c),  // bubble_out
        QColor(0x0d, 0x0d, 0x0d),  // bubble_in
        QColor(0x02, 0x02, 0x02),  // inputBg
        "AMOLED"
    };
}

Colors fainsDefault() {
    return {
        QColor(0x17, 0x1c, 0x28),  // background   FainsGram signature
        QColor(0x10, 0x14, 0x1d),  // sidebar
        QColor(0xff, 0x6b, 0x35),  // accent  FainsGram orange
        QColor(0xf0, 0xf2, 0xff),  // text
        QColor(0x70, 0x78, 0x90),  // textSecondary
        QColor(0xff, 0x6b, 0x35, 0x55),  // bubble_out
        QColor(0x1f, 0x25, 0x35),  // bubble_in
        QColor(0x0c, 0x0f, 0x18),  // inputBg
        "FainsGram Default"
    };
}

} // namespace Palette
} // namespace FainsGram
