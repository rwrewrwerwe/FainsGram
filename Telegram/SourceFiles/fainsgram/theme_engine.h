/*
 * FainsGram — Theme Engine
 * =========================
 * Handles all UI customization:
 *   • Custom wallpaper (PNG/JPG/GIF/WebP)
 *   • Custom font (system font or .ttf/.otf file)
 *   • Custom app icon (macOS .icns, no recompile)
 *   • Built-in themes: Ghost Dark, Neon, Minimal, AMOLED
 *   • Import standard .tdesktop-theme files
 *
 * Copyright (c) 2024 FainsGram. MIT License.
 */

#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtGui/QFont>
#include <QtGui/QColor>

namespace FainsGram {

enum class BuiltinTheme {
    None = 0,
    GhostDark,   // Deep dark slate + ghost blue accents
    Neon,        // Black + electric purple/green neon
    Minimal,     // Off-white, clean, minimal
    Amoled,      // Pure #000000 background for OLED screens
    FainsDefault // FainsGram branded dark theme
};

struct ThemeSettings {
    BuiltinTheme builtinTheme  = BuiltinTheme::FainsDefault;
    QString      wallpaperPath;      // Custom wallpaper file path
    bool         wallpaperBlur  = false;
    float        wallpaperOpacity = 0.8f;
    bool         useCustomFont  = false;
    QString      fontFamily     = "Inter";
    int          fontSize       = 13;
    QString      customFontPath;     // .ttf/.otf file path
    QString      customIconPath;     // .icns (macOS)
    QString      customThemePath;    // .tdesktop-theme file
    bool         animatedWallpaper = false;
};

class ThemeEngine final : public QObject {
    Q_OBJECT
public:
    explicit ThemeEngine(QObject* parent = nullptr);

    void loadSettings();
    void saveSettings();
    void apply();  // Apply all settings to the running UI

    // Wallpaper
    void setWallpaper(const QString& path);
    void clearWallpaper();
    void setWallpaperBlur(bool on);
    void setWallpaperOpacity(float v);

    // Font
    void setCustomFont(const QString& family, int size = 13);
    void setCustomFontFile(const QString& ttfPath);
    void resetFont();

    // App icon (macOS only)
    void setAppIcon(const QString& icnsPath);
    void resetAppIcon();

    // Built-in themes
    void applyBuiltinTheme(BuiltinTheme t);
    BuiltinTheme currentTheme() const { return _s.builtinTheme; }

    // Custom .tdesktop-theme
    void importThemeFile(const QString& path);

    const ThemeSettings& settings() const { return _s; }

Q_SIGNALS:
    void themeChanged();
    void wallpaperChanged(const QString& path);
    void fontChanged(const QFont& font);

private:
    void applyWallpaper();
    void applyFont();
    void applyIcon();
    void applyBuiltinColors(BuiltinTheme t);

    ThemeSettings _s;
};

// ── Built-in theme colour palettes ──────────────────────────────────────────

namespace Palette {

struct Colors {
    QColor background;
    QColor sidebar;
    QColor accent;
    QColor text;
    QColor textSecondary;
    QColor bubble_out;   // Own message bubble
    QColor bubble_in;    // Incoming bubble
    QColor inputBg;
    QString name;
};

Colors ghostDark();
Colors neon();
Colors minimal();
Colors amoled();
Colors fainsDefault();

} // namespace Palette
} // namespace FainsGram
