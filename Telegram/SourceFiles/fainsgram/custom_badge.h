/*
 * FainsGram — Custom Badge
 * =========================
 * Displays ⚡ badge next to usernames of other FainsGram users.
 * Also handles visual phone number override in your own profile.
 *
 * Detection: FainsGram users put a hidden marker in their Bio field
 * (configurable, default: "#FG" tag at end) — this is purely voluntary.
 * Alternative: bot-based verification API (see fainsgram_verify_bot/).
 *
 * Copyright (c) 2024 FainsGram. MIT License.
 */

#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QSet>

namespace FainsGram {

class CustomBadge final : public QObject {
    Q_OBJECT
public:
    explicit CustomBadge(QObject* parent = nullptr);

    void loadSettings();
    void saveSettings();

    // Badge display
    bool badgesEnabled() const { return _enabled; }
    void setBadgesEnabled(bool on);

    QString badgeEmoji() const { return _badge; }
    void    setBadgeEmoji(const QString& emoji);

    // Bio marker used to identify FainsGram users
    QString bioMarker() const { return _marker; }
    void    setBioMarker(const QString& m);

    // Check if a user's bio contains our marker
    bool isFainsGramUser(const QString& bio) const;

    // Cache of known FainsGram user IDs (populated when bio detected)
    void   markUser(qint64 userId);
    bool   isKnownUser(qint64 userId) const;
    void   forgetUser(qint64 userId);

    // Your own marker — appended to bio when you enable the badge
    bool  selfMarkEnabled() const { return _selfMark; }
    void  setSelfMarkEnabled(bool on);

Q_SIGNALS:
    void badgeSettingsChanged();

private:
    bool    _enabled  = true;
    QString _badge    = "⚡";
    QString _marker   = "#FG";
    bool    _selfMark = true;
    QSet<qint64> _knownUsers;
};

} // namespace FainsGram
